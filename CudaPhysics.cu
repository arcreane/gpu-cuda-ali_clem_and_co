#include <cuda_runtime.h>
// #include <helper_cuda.h> // Cet include pose pb 
#include <cmath>
#include <thrust/sort.h>
#include <thrust/device_vector.h>
#include <thrust/execution_policy.h>
#include <thrust/pair.h>
#include <thrust/iterator/zip_iterator.h> // Souvent nécessaire pour les algorithmes par clé
#include <thrust/transform.h>     // Algorithmes de base

// Définition des constantes de simulation (à synchroniser avec Qt si nécessaire)
// Pour l'interaction souris
#define INTERACTION_RADIUS 150.0f
#define FORCE_FACTOR 0.6f
#define ATTRACTION_SCALING_FACTOR 2.0f
#define CELL_SIZE 6.0f // à harmoniser avec le code global
#define GRID_WIDTH 1337 // Largeur d'une grande grille pour éviter les collisions de hash
#define PARTICLE_RADIUS 1.5f // Rayon des particules supposé 1.5f (taille de 3px)
#define MIN_DIST (PARTICLE_RADIUS * 2.0f) // Distance minimale pour la collision inter-particules
#define MIN_DIST_SQ (MIN_DIST * MIN_DIST) // Distance minimale au carré

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
// IV. KERNEL 4 : Calcul du Hash (Hashing)
// ----------------------------------------------------

__global__ void calculateHashKernel(
    const float* pos_x, const float* pos_y,
    int* grid_hash, int* particle_index,
    int numParticles, int gridTotalWidth, int gridTotalHeigth)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;

    if (i < numParticles){
        // 1. Déterminer les coordonnées de la cellule
        // Bien s'assurer que les positions sont dans les limites [0, width/height]
        int gx = (int)(pos_x[i] / CELL_SIZE);
        int gy = (int)(pos_y[i] / CELL_SIZE);

        // Sécurité des coordonnées de la grille
        if (gx < 0) gx = 0;
        if (gy < 0) gy = 0;

        // 2. Calculer l'Index de Hachage (Hash)
        // Utilisation d'un décalage (shift) pour s'assurer que l'index est unique
        // sans avoir à connaître la taille exacte de la grille de rendu.
        int hash = (gy * GRID_WIDTH) + gx;

        // 3. Enregistrement
        grid_hash[i] = hash;
        particle_index[i] = i; // L'index original (0, 1, 2, 3...)

    }

}

// --------------------------------------------------------
// V. KERNEL 5 : Résolution des Collisions Inter-Particules
// --------------------------------------------------------
__global__ void resolveCollisionsKernel(
    float* pos_x, float* pos_y, 
    float* vel_x, float* vel_y,
    const int* particle_index, // Index triés (qui pointe vers les vrais tableaux)
    const int* grid_hash,      // Hash trié (pour vérifier les voisins)
    // const int* cell_starts,    // Index de début de chaque nouvelle cellule de hash mais non utilisé depuis l'implémentation simple
    int numParticles,
    float bounciness
)
{
    int thread_idx = blockIdx.x * blockDim.x + threadIdx.x;

    if (thread_idx < numParticles) {
        // L'index I est l'index dans le tableau trié (0 à N-1)
        int i = thread_idx;
        
        // p1_original_idx est l'index réel de la particule dans les tableaux (0 à N-1)
        int p1_original_idx = particle_index[i]; 

        // Pour simplifier au maximum, vérifions juste la cellule actuelle (pas les 8 voisines)
        // La première particule de la cellule (début de la boucle)
        int start_of_cell = i;
        // On remonte jusqu'à trouver le début de la cellule triée (où le hash change)
        while (start_of_cell > 0 && grid_hash[start_of_cell - 1] == grid_hash[i]) {
            start_of_cell--;
        }
        
        // La dernière particule de la cellule (fin de la boucle)
        int end_of_cell = i;
        // On descend jusqu'à trouver la fin de la cellule triée
        while (end_of_cell < numParticles - 1 && grid_hash[end_of_cell + 1] == grid_hash[i]) {
            end_of_cell++;
        }

        // Boucle de collision : Vérifier p1 contre toutes les autres particules p2
        // dans la MÊME cellule. On commence à i + 1 pour éviter la double vérification.
        for (int j = i + 1; j <= end_of_cell; ++j) {
            
            int p2_original_idx = particle_index[j];

            // Ne jamais vérifier une particule contre elle-même
            if (p1_original_idx == p2_original_idx) continue;

            // Chargement des positions et vitesses des deux particules
            float p1x = pos_x[p1_original_idx];
            float p1y = pos_y[p1_original_idx];
            float p2x = pos_x[p2_original_idx];
            float p2y = pos_y[p2_original_idx];
            
            float v1x = vel_x[p1_original_idx];
            float v1y = vel_y[p1_original_idx];
            float v2x = vel_x[p2_original_idx];
            float v2y = vel_y[p2_original_idx];

            // --- Calcul de la Collision ---
            float dx = p1x - p2x;
            float dy = p1y - p2y;
            float distSq = dx*dx + dy*dy;

            // COLLISION DÉTECTÉE !
            if (distSq < MIN_DIST_SQ && distSq > 0.001f) {
                float dist = sqrtf(distSq);
                
                // 1. Repousser les particules pour qu'elles ne se chevauchent pas
                float overlap = (MIN_DIST - dist) * 0.5f; 
                float nx = dx / dist; // Vecteur normal normalisé
                float ny = dy / dist;

                // On applique la répulsion aux tableaux réels
                pos_x[p1_original_idx] += nx * overlap;
                pos_y[p1_original_idx] += ny * overlap;
                pos_x[p2_original_idx] -= nx * overlap;
                pos_y[p2_original_idx] -= ny * overlap;

                // 2. Échange d'énergie (Rebond)
                float vRelX = v1x - v2x;
                float vRelY = v1y - v2y;
                float velAlongNormal = vRelX * nx + vRelY * ny;

                // Si elles s'éloignent déjà, on ne fait rien
                if (velAlongNormal > 0) continue;

                // Application de l'impulsion (masse égale = 1)
                float j = -(1.0f + bounciness) * velAlongNormal / 2.0f; 

                float impulseX = j * nx;
                float impulseY = j * ny;

                // Application de l'impulsion aux vitesses réelles
                vel_x[p1_original_idx] += impulseX;
                vel_y[p1_original_idx] += impulseY;
                vel_x[p2_original_idx] -= impulseX;
                vel_y[p2_original_idx] -= impulseY;
            }
        }
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
//static int* d_cell_starts = nullptr;
static int* d_grid_hash = nullptr;
static int* d_particle_index = nullptr;
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
        if (d_grid_hash) cudaFree(d_grid_hash);
        if (d_particle_index) cudaFree(d_particle_index);
        //if (d_cell_starts) cudaFree(d_cell_starts);
        
        // Allocation de la nouvelle mémoire
        cudaMalloc((void**)&d_pos_x, size);
        cudaMalloc((void**)&d_pos_y, size);
        cudaMalloc((void**)&d_vel_x, size);
        cudaMalloc((void**)&d_vel_y, size);
        cudaMalloc((void**)&d_grid_hash, numParticles * sizeof(int)); // Allocation de mémoire pour les tableaux de hashage et de l'index
        cudaMalloc((void**)&d_particle_index, numParticles * sizeof(int));
       //cudaMalloc((void**)&d_cell_starts, numParticles * sizeof(int)); // Allocation généreuse car la taille finale sera plus petite
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

    // ------------------------------------------------------------------
    // PHASE PRÉLIMINAIRE : HACHAGE ET TRI
    // ------------------------------------------------------------------

    // Définition des constantes de la grille pour le Kernel
    const int GRID_TOTAL_WIDTH = 1000; // Taille de la grille (doit être assez grande)
    const int GRID_TOTAL_HEIGHT = 1000;

    // ÉTAPE 1 : Calcul du Hash et de l'Index Original (Kernel)
    calculateHashKernel<<<blocksPerGrid, threadsPerBlock>>>(
        d_pos_x, d_pos_y,
        d_grid_hash, d_particle_index,
        numParticles, GRID_TOTAL_WIDTH, GRID_TOTAL_HEIGHT
        );

    // ÉTAPE 2 : Tri des tableaux par la clé de hachage (Thrust)
    thrust::sort_by_key(
        thrust::device,
        d_grid_hash,
        d_grid_hash + numParticles,
        d_particle_index
        );

    /*
    // Déclaration des pointeurs de fin (résultats de l'opération unique_by_key)
    int* end_keys_ptr;
    int* end_values_ptr;

    // ÉTAPE 3 : Calculer les index de début de chaque cellule
    // Utilisation d'un vecteur temporaire pour stocker les index de fin
    thrust::device_vector<int> unique_hashes_temp(numParticles);

    // unique_by_key renvoie un pointeur vers la fin de la nouvelle séquence.
    // L'algorithme écrit l'index de début de chaque groupe dans d_cell_starts
    thrust::pair<int*, int*> result = thrust::unique_by_key(
        thrust::device,
        d_grid_hash, // Clés triées
        d_grid_hash + numParticles,
        d_particle_index, // Valeurs triées
        unique_hashes_temp.begin(), // Pour stocker les hashs uniques (non utilisé, mais nécessaire)
        d_cell_starts // OÙ stocker les index de DÉBUT de chaque groupe
        &end_keys_ptr,   // Pointeur pour le résultat (le nouveau pointeur de fin des clés)
        &end_values_ptr  // Pointeur pour le résultat (le nouveau pointeur de fin des valeurs)
        );

    // Le nombre de cellules uniques est la distance jusqu'au pointeur de fin
    int numUniqueCells = thrust::distance(d_cell_starts, result.second);
    // Note: On devrait stocker ce numUniqueCells statiquement pour un usage ultérieur.
    */

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
    
    // ------------------------------------------------------------------
    // ÉTAPE DE RÉSOLUTION DES COLLISIONS
    // ------------------------------------------------------------------

    // 1. Calcul des index de début de cellule (Utilise Thrust::unique_by_key)
    // Cette opération est nécessaire pour la suite et génère un tableau plus petit.
    // Cependant, pour la méthode simple du KERNEL ci-dessus (qui trouve le début/fin
    // manuellement), cette étape est facultative, mais elle est le standard de l'optimisation.
    // Pour l'instant, nous nous en passons car la vérification manuelle dans le Kernel est plus simple
    // pour éviter d'introduire des variables d'état (d_cell_starts) compliquées.

    // KERNEL 4 : Résolution des collisions inter-particules
    resolveCollisionsKernel<<<blocksPerGrid, threadsPerBlock>>>(
        d_pos_x, d_pos_y, d_vel_x, d_vel_y,
        d_particle_index, // Le tableau trié
        d_grid_hash,      // Le hash trié
        // d_cell_starts,    // (Non utilisé dans cette version simple du Kernel)
        numParticles,
        bounciness
    );

    // Synchronisation pour s'assurer que tous les Kernels sont terminés
    cudaDeviceSynchronize();

    // ... (Transfert Périphérique -> Hôte inchangé) ...
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
