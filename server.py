# Cellule 4 : Script du Serveur Python (server.py)
import os
import json
import numpy as np
from flask import Flask, request, jsonify
from ctypes import cdll, POINTER, c_float, c_int, c_bool

# --- 1. CHARGEMENT DE LA LIBRAIRIE CUDA COMPILÉE ---
try:
    # Charge la librairie partagée créée par nvcc
    # NOTE: Le chemin peut varier selon la structure de notre dépôt
    cuda_lib = cdll.LoadLibrary('./libCudaPhysics.so')
    
    # Définition des types d'arguments de la fonction C (c'est essentiel!)
    cuda_lib.run_cuda_simulation.argtypes = [
        POINTER(c_float), POINTER(c_float),  # pos_x/pos_y
        POINTER(c_float), POINTER(c_float),  # vel_x/vel_y
        c_int, c_float, c_float, c_int, c_int,
        c_float, c_float, c_float, c_bool,
        c_int # interactionRadius
    ]
except Exception as e:
    print(f"Erreur de chargement CUDA: {e}")
    # Ajoutez ici une gestion d'erreur ou quittez

# --- 2. LOGIQUE DU SERVEUR FLASK ---
app = Flask(__name__)

@app.route('/simulate', methods=['POST'])
def simulate():
    if not cuda_lib:
        return jsonify({"error": "CUDA Engine not loaded"}), 500

    data = request.get_json()

    # 2.1 Désérialisation des données
    num_particles = data['num']
    # Initialisation des tableaux numpy pour stocker les données
    pos_x = np.array([p['px'] for p in data['particles']], dtype=np.float32)
    pos_y = np.array([p['py'] for p in data['particles']], dtype=np.float32)
    vel_x = np.array([p['vx'] for p in data['particles']], dtype=np.float32)
    vel_y = np.array([p['vy'] for p in data['particles']], dtype=np.float32)



    # 2.2 Appel de la fonction C/CUDA
    # Conversion des tableaux numpy en pointeurs C pour les passer à la librairie compilée
    cuda_lib.run_cuda_simulation(
        pos_x.ctypes.data_as(POINTER(c_float)),
        pos_y.ctypes.data_as(POINTER(c_float)),
        vel_x.ctypes.data_as(POINTER(c_float)),
        vel_y.ctypes.data_as(POINTER(c_float)),
        c_int(num_particles),
        c_float(data['friction']),
        c_float(data['bounciness']),
        c_int(data['width']),
        c_int(data['height']),
        c_float(data['mouse_x']),
        c_float(data['mouse_y']),
        c_float(data['mouse_speed']),
        c_bool(data['attraction_mode']),
        c_int(150) # interactionRadius (valeur en dur dans votre code Qt)
    )

    # On commente la lib cuda pour voir si le pb vient de Cuda ou python
    # 2.3 Sérialisation des résultats
    result_particles = []
    for i in range(num_particles):
        result_particles.append({
            "px": float(pos_x[i]),
            "py": float(pos_y[i]),
            "vx": float(vel_x[i]),
            "vy": float(vel_y[i])
        })

    return jsonify({"particles": result_particles})

if __name__ == '__main__':
    # Flask s'exécute sur le port par défaut (5000)
    # L'exécution dans Colab nécessite un thread séparé ou des options spécifiques
    app.run(host='0.0.0.0', port=5000)
