# PontConnect Sensor Injector

## Description

Ce script permet de collecter des données de capteurs (température, humidité, niveau d'eau) depuis un broker MQTT et de les transmettre automatiquement à l'API REST PontConnect. Il gère l'installation des dépendances requises, l'authentification à l'API et le traitement des données selon le format attendu.

## Fonctionnalités

- Installation automatique des dépendances (mosquitto-clients, jq, curl)
- Connexion au broker MQTT local ou distant
- Décodage des payloads binaires des capteurs
- Authentification automatique à l'API REST
- Création automatique de compte si nécessaire
- Gestion de la réauthentification en cas d'expiration du token
- Support de plusieurs types de capteurs (température, humidité, niveau d'eau)

## Prérequis

- Système d'exploitation basé sur Debian/Ubuntu avec `apt`
- Accès à Internet pour l'installation des dépendances
- Connexion au broker MQTT configuré
  - Par défaut déjà configuré à l'installation de ChirpStack
- Droits sudo pour l'installation des dépendances

## Installation

1. Téléchargez le script
2. Rendez-le exécutable:

   ```bash
   chmod +x sensor-data-collectore.sh
   ```

3. Exécutez-le:

   ```bash
   ./sensor-data-collectore.sh
   ```

Le script installera automatiquement les dépendances manquantes au premier lancement.

## Configuration

Modifiez les paramètres suivants au début du script selon vos besoins :

```bash
# CONFIGURATION MODIFIABLE
API_URL="https://votre-api-url.com/api"      # URL de l'API PontConnect
TOKEN_FILE="/tmp/pontconnect_token"          # Emplacement du fichier stockant le token
USER_EMAIL="votre.email@exemple.com"         # Email pour l'authentification
USER_PASSWORD="votre_mot_de_passe"           # Mot de passe pour l'authentification
BROKER="localhost"                           # Adresse du broker MQTT
PORT=1883                                    # Port MQTT standard (non-TLS)
APPLICATION_ID="10989edf-2a96-4daf-xxxx"     # ID de votre application ChirpStack
DEVICE_EUI="cd2e7dd7e5c6de03"                # Identifiant unique de votre appareil
TOPIC="application/+/device/+/event/up"      # Topic MQTT pour les données montantes
```

### Comment trouver votre APPLICATION_ID

L'identifiant d'application peut être trouvé dans l'interface ChirpStack :

1. Connectez-vous à votre interface ChirpStack
2. Naviguez vers la section "Applications"
3. Sélectionnez votre application
4. Observez l'URL dans votre navigateur : `.../applications/VOTRE_APPLICATION_ID`

### Comment trouver votre DEVICE_EUI

L'identifiant de l'appareil (EUI) est disponible :

- Dans la section "Appareils" de votre application ChirpStack
- Directement sur l'appareil physique ou sa documentation
- Dans les détails de configuration de l'appareil dans l'interface ChirpStack

## Structure des données

Le script s'attend à recevoir des messages MQTT avec un payload encodé en base64 contenant:

- Bytes 0-1: Température (int16, centièmes de °C)
- Bytes 2-3: TDS/Humidité (uint16, ppm)
- Bytes 4-5: Niveau d'eau (uint16, dixièmes de cm)

Ces données sont décodées et converties dans les bonnes unités avant d'être envoyées à l'API.

## Fonctionnement

```
┌───────────────┐     ┌──────────────┐     ┌───────────────┐
│  CAPTEURS IoT │────▶│ BROKER MQTT  │────▶│ mqtt_to_db.sh │
└───────────────┘     └──────────────┘     └───────┬───────┘
                                                   │
                                                   ▼
                                           ┌───────────────┐
                                           │   API REST    │
                                           │  PontConnect  │
                                           └───────────────┘
```

## Dépannage

### Erreurs courantes

- **apt n'est pas disponible**: Ce script nécessite un système basé sur Debian/Ubuntu.
- **Échec de l'installation de [outil]**: Vérifiez votre connexion Internet et les permissions.
- **Erreur d'authentification**: Vérifiez les identifiants USER_EMAIL et USER_PASSWORD.
- **Pas de données reçues**: Vérifiez la connexion au broker MQTT et les paramètres APPLICATION_ID et DEVICE_EUI.

### Stockage du token

Le token d'authentification est stocké dans le fichier défini par TOKEN_FILE (par défaut `/tmp/pontconnect_token`). Si vous rencontrez des problèmes d'authentification, vous pouvez supprimer ce fichier pour forcer une nouvelle authentification:

```bash
rm /tmp/pontconnect_token
```

## Sécurité

⚠️ Le mot de passe est stocké en texte clair dans le script. Pour une utilisation en production, envisagez d'utiliser des variables d'environnement ou un stockage sécurisé des identifiants.
