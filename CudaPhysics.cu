#include <cuda_runtime.h>
#include <helper_cuda.h>
#include <cmath>

// Définition des constantes de simulation (à synchroniser avec Qt si nécessaire)
// Pour l'interaction souris
#define INTERACTION_RADIUS 150.0f
#define FORCE_FACTOR 0.6f
#define ATTRACTION_SCALING_FACTOR 2.0f

// ----------------------------------------------------
// I. KERNEL 1 : Application des forces, Mouvement et Frottement
// ----------------------------------------------------

__global__ void applyForcesAndMoveKernel(
    float* pos_x, float* pos_y, 
    float* vel_x, float* vel_y,
    int numParticles,
    float friction,
    float mousePosX, float mousePosY, float mouseSpeed, 
    bool isBlackHoleActive
)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;

    if (i < numParticles) {
        float px = pos_x[i];
        float py = pos_y[i];
        float vx = vel_x[i];
        float vy = vel_y[i];
        
        // --- A. Interaction Souris (Forces) ---
        float dx = px - mousePosX;
        float dy = py - mousePosY;
        float distSq = dx*dx + dy*dy;
        float distMouse = sqrtf(distSq);

        if (distMouse < INTERACTION_RADIUS && distMouse > 1.0f) {
            float dirX = dx / distMouse;
            float dirY = dy / distMouse;
            
            float interactionRatio = (1.0f - (distMouse / INTERACTION_RADIUS));

            if (isBlackHoleActive) {
                // Attraction (Trou Noir)
                float attraction = interactionRatio * ATTRACTION_SCALING_FACTOR;
                vx -= dirX * attraction;
                vy -= dirY * attraction;
            } else if (mouseSpeed > 0.1f) {
                // Répulsion (Balayage)
                float repulsion = interactionRatio * mouseSpeed * FORCE_FACTOR;
                vx += dirX * repulsion;
                vy += dirY * repulsion;
            }
        }

        // --- B. Application Mouvement et Frottement ---
        px += vx;
        py += vy;
        
        vx *= friction;
        vy *= friction;

        // Sauvegarde des nouvelles valeurs
        pos_x[i] = px;
        pos_y[i] = py;
        vel_x[i] = vx;
        vel_y[i] = vy;
    }
}

// ----------------------------------------------------
// II. KERNEL 2 : Collisions Murs (Horizontal)
// ----------------------------------------------------

__global__ void wallCollisionXKernel(
    float* pos_x, float* vel_x,
    int numParticles,
    int width
    // Note: Le coefficient de restitution (bounciness) peut être appliqué ici
)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;

    if (i < numParticles) {
        float px = pos_x[i];
        float vx = vel_x[i];
        float bounciness = 0.9f; // Valeur en dur (le coefficient de restitution doit être passé en paramètre si on veut le changer)
        
        if (px < 0) { 
            px = 0; 
            vx = -vx * bounciness; 
        } else if (px > width) { 
            px = (float)width; 
            vx = -vx * bounciness; 
        }

        pos_x[i] = px;
        vel_x[i] = vx;
    }
}

// ----------------------------------------------------
// III. KERNEL 3 : Collisions Murs (Vertical)
// ----------------------------------------------------

__global__ void wallCollisionYKernel(
    float* pos_y, float* vel_y,
    int numParticles,
    int height
)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;

    if (i < numParticles) {
        float py = pos_y[i];
        float vy = vel_y[i];
        float bounciness = 0.9f; // Valeur en dur (le coefficient de restitution doit être passé en paramètre si on veut le changer)
        
        if (py < 0) { 
            py = 0; 
            vy = -vy * bounciness; 
        } else if (py > height) { 
            py = (float)height; 
            vy = -vy * bounciness; 
        }

        pos_y[i] = py;
        vel_y[i] = vy;
    }
}

// ----------------------------------------------------
// IV. KERNEL 4 : Collision inter-particules (VIDE pour le moment)
// ----------------------------------------------------
/*
__global__ void interParticleCollisionKernel(...)
{
    // Ce Kernel sera implémenté dans la phase 5 (la plus complexe)
}
*/

// ----------------------------------------------------
// V. POINT D'ENTRÉE POUR PYTHON (extern "C")
// ----------------------------------------------------

// Définition de la mémoire persistante pour les pointeurs GPU
// L'utilisation de ces tableaux est cruciale pour que Ctypes puisse les manipuler
static float* d_pos_x = nullptr;
static float* d_pos_y = nullptr;
static float* d_vel_x = nullptr;
static float* d_vel_y = nullptr;
static int s_allocated_count = 0;

// Fonction C exposée pour l'appel par la librairie Python (ctypes)
extern "C" void run_cuda_simulation(
    float* host_pos_x, float* host_pos_y, 
    float* host_vel_x, float* host_vel_y, 
    int numParticles, 
    float friction, float bounciness, 
    int width, int height,
    float mousePosX, float mousePosY, float mouseSpeed, 
    bool isBlackHoleActive,
    int interactionRadius // Pour la cohérence du prototype
)
{
    size_t size = numParticles * sizeof(float);
    
    // 1. Gestion de l'Allocation/Réallocation de la Mémoire GPU
    if (numParticles != s_allocated_count) {
        // Libérer l'ancienne mémoire si elle existe
        if (d_pos_x) cudaFree(d_pos_x);
        if (d_pos_y) cudaFree(d_pos_y);
        if (d_vel_x) cudaFree(d_vel_x);
        if (d_vel_y) cudaFree(d_vel_y);
        
        // Allocation de la nouvelle mémoire (4 tableaux séparés)
        cudaMalloc((void**)&d_pos_x, size);
        cudaMalloc((void**)&d_pos_y, size);
        cudaMalloc((void**)&d_vel_x, size);
        cudaMalloc((void**)&d_vel_y, size);
        s_allocated_count = numParticles;
    }
    
    // 2. Transfert Hôte (NumPy) -> Périphérique (GPU)
    cudaMemcpy(d_pos_x, host_pos_x, size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_pos_y, host_pos_y, size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_vel_x, host_vel_x, size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_vel_y, host_vel_y, size, cudaMemcpyHostToDevice);
    
    // Définition de la grille de lancement
    int threadsPerBlock = 256;
    int blocksPerGrid = (numParticles + threadsPerBlock - 1) / threadsPerBlock;
    
    // 3. Lancement des Kernels (Exécution Parallèle)
    
    // KERNEL 1 : Application des forces, mouvement et frottement
    applyForcesAndMoveKernel<<<blocksPerGrid, threadsPerBlock>>>(
        d_pos_x, d_pos_y, d_vel_x, d_vel_y,
        numParticles, friction,
        mousePosX, mousePosY, mouseSpeed, 
        isBlackHoleActive
    );
    
    // KERNEL 2 : Collisions Murs (Horizontal)
    wallCollisionXKernel<<<blocksPerGrid, threadsPerBlock>>>(
        d_pos_x, d_vel_x,
        numParticles, width
    );

    // KERNEL 3 : Collisions Murs (Vertical)
    wallCollisionYKernel<<<blocksPerGrid, threadsPerBlock>>>(
        d_pos_y, d_vel_y,
        numParticles, height
    );
    
    // Nous sautons KERNEL 4 pour la collision inter-particules
    
    // Synchronisation pour s'assurer que tous les Kernels sont terminés
    cudaDeviceSynchronize();
    
    // 4. Transfert Périphérique (GPU) -> Hôte (NumPy)
    // Les résultats sont rapatriés dans les tableaux NumPy initiaux
    cudaMemcpy(host_pos_x, d_pos_x, size, cudaMemcpyDeviceToHost);
    cudaMemcpy(host_pos_y, d_pos_y, size, cudaMemcpyDeviceToHost);
    cudaMemcpy(host_vel_x, d_vel_x, size, cudaMemcpyDeviceToHost);
    cudaMemcpy(host_vel_y, d_vel_y, size, cudaMemcpyDeviceToHost);
    
    // 5. Nettoyage
    // NOTE : Nous ne faisons pas de cudaFree ici car la mémoire est persistante.
}