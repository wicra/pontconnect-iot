#!/bin/bash

# CONFIGURATION DES COULEURS
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# CONFIGURATION MODIFIABLE
API_URL="VOTRE_API_URL" # Remplacez par l'URL de votre API
TOKEN_FILE="/tmp/pontconnect_token"
USER_EMAIL="user1@gmail.com"
USER_PASSWORD="12345678901234567890"
BROKER="localhost"
PORT=1883
APPLICATION_ID="10989edf-2a96-4daf-baf8-a221c0cff7e1"
DEVICE_EUI="cd2e7dd7e5c6de03" 
TOPIC="application/+/device/+/event/up"

# MAPPAGE DES CAPTEURS
declare -A CAPTEURS=(
  ["temperature"]="1"
  ["turbinite"]="2"
  ["niveau_eau"]="3"
  ["humidite"]="4" 
)

# AFFICHAGE DU BANNER
show_banner() {
  clear
  echo -e "${BLUE}╔═════════════════════════════════════════════════════════════╗${NC}"
  echo -e "${BLUE}║          ${GREEN}PontConnect Sensor ${BLUE}                 ║${NC}"
  echo -e "${BLUE}╚═════════════════════════════════════════════════════════════╝${NC}"
  echo
}

# FONCTION DE LOG SIMPLIFIÉE
log_message() {
  local level=$1
  local message=$2
  local timestamp=$(date "+%Y-%m-%d %H:%M:%S")
  
  case "$level" in
    "INFO") color=$BLUE ;;
    "SUCCESS") color=$GREEN ;;
    "ERROR") color=$RED ;;
    *) color=$NC ;;
  esac
  
  # AFFICHAGE DANS LE TERMINAL
  echo -e "${color}[$timestamp] [$level] $message${NC}"
}

# FONCTION POUR INSTALLER LES DÉPENDANCES
install_dependencies() {
  log_message "INFO" "Vérification des dépendances..."
  
  # LISTE DES PAQUETS ET OUTILS ASSOCIÉS
  declare -A tools=(
    ["mosquitto_sub"]="mosquitto-clients"
    ["jq"]="jq"
    ["curl"]="curl"
  )
  
  # LISTE DES PAQUETS À INSTALLER
  packages_to_install=()
  
  # VÉRIFIER QUE APT EST DISPONIBLE
  if ! command -v apt &> /dev/null; then
    log_message "ERROR" "apt n'est pas disponible sur ce système"
    exit 1
  fi
  
  # VÉRIFICATION DES OUTILS MANQUANTS
  for tool in "${!tools[@]}"; do
    if ! command -v "$tool" &> /dev/null; then
      log_message "INFO" "$tool non trouvé, sera installé"
      packages_to_install+=("${tools[$tool]}")
    fi
  done
  
  # INSTALLATION DES PAQUETS MANQUANTS SI NÉCESSAIRE
  if [ ${#packages_to_install[@]} -gt 0 ]; then
    log_message "INFO" "Installation des paquets: ${packages_to_install[*]}"
    sudo apt update
    sudo apt install -y "${packages_to_install[@]}"
    
    # VÉRIFICATION POST-INSTALLATION
    for tool in "${!tools[@]}"; do
      if ! command -v "$tool" &> /dev/null; then
        log_message "ERROR" "Échec de l'installation de $tool"
        exit 1
      fi
    done
    log_message "SUCCESS" "Installation terminée"
  fi
}

# FONCTION D'AUTHENTIFICATION
authenticate() {
  log_message "INFO" "Authentification..."
  
  # TENTE DE SE CONNECTER
  response=$(curl -s -X POST "$API_URL/auth/login" \
    -H "Content-Type: application/json" \
    -d "{\"email\":\"$USER_EMAIL\",\"password\":\"$USER_PASSWORD\"}")
  
  # VÉRIFIER SI LA RÉPONSE EST UN JSON VALIDE
  if ! echo "$response" | jq -e . >/dev/null 2>&1; then
    log_message "ERROR" "Réponse invalide: $response"
    return 1
  fi
  
  # VÉRIFIE SI LA CONNEXION A RÉUSSI
  success=$(echo "$response" | jq -r '.success')
  
  if [ "$success" == "true" ]; then
    # EXTRAIT ET SAUVEGARDE LE TOKEN
    token=$(echo "$response" | jq -r '.token')
    echo "$token" > "$TOKEN_FILE"
    log_message "SUCCESS" "Authentification réussie"
    return 0
  else
    log_message "INFO" "Compte inexistant, tentative de création..."
    
    # TENTATIVE D'INSCRIPTION AVEC LES MÊMES IDENTIFIANTS
    register_response=$(curl -s -X POST "$API_URL/auth/register" \
      -H "Content-Type: application/json" \
      -d "{\"name\":\"$USER_EMAIL\",\"email\":\"$USER_EMAIL\",\"password\":\"$USER_PASSWORD\"}")
    
    reg_success=$(echo "$register_response" | jq -r '.success')
    
    if [ "$reg_success" == "true" ]; then
      log_message "SUCCESS" "Compte créé, nouvel essai d'authentification"
      # RÉESSAYER L'AUTHENTIFICATION
      authenticate
      return $?
    else
      log_message "ERROR" "Échec d'enregistrement: $(echo "$register_response" | jq -r '.message')"
      return 1
    fi
  fi
}

# FONCTION POUR ENVOYER LES DONNÉES DES CAPTEURS
send_mesure_data() {
  local capteur_id=$1
  local valeur=$2
  
  # RÉCUPÉRER LE NOM DU CAPTEUR À PARTIR DE L'ID
  local type_capteur
  for key in "${!CAPTEURS[@]}"; do
    if [[ "${CAPTEURS[$key]}" == "$capteur_id" ]]; then
      type_capteur=$key
      break
    fi
  done
  
  log_message "INFO" "Envoi: Capteur $type_capteur = $valeur"
  
  # VÉRIFIER SI LE TOKEN EXISTE
  if [ ! -f "$TOKEN_FILE" ]; then
    log_message "INFO" "Pas de token, authentification..."
    authenticate || return 1
  fi
  
  # RÉCUPÈRE LE TOKEN
  TOKEN=$(cat "$TOKEN_FILE")
  
  # ENVOI DES DONNÉES
  response=$(curl -s -X POST "$API_URL/sensor/mesures" \
    -H "Content-Type: application/json" \
    -H "Authorization: Bearer $TOKEN" \
    -d "{\"capteur_id\": $capteur_id, \"valeur\": $valeur}")
  
  # VÉRIFIER SI LA RÉPONSE EST VALIDE
  if echo "$response" | jq -e '.success == true' &>/dev/null; then
    log_message "SUCCESS" "Données envoyées: $type_capteur = $valeur"
    return 0
  else
    # SI TOKEN EXPIRÉ OU AUTRE ERREUR D'AUTHENTIFICATION
    if [[ "$response" == *"Unauthorized"* ]] || [[ "$response" == *"expiré"* ]]; then
      log_message "INFO" "Session expirée, nouvelle authentification..."
      authenticate && send_mesure_data "$capteur_id" "$valeur"
      return $?
    else
      log_message "ERROR" "Échec d'envoi: $(echo "$response" | jq -r '.message // "Erreur inconnue"')"
      return 1
    fi
  fi
}

# SCRIPT PRINCIPAL

# AFFICHER LA BANNIÈRE
show_banner

# EXÉCUTER L'INSTALLATION AUTOMATIQUE DES DÉPENDANCES
install_dependencies

# AUTHENTIFICATION INITIALE
authenticate || exit 1

# DÉMARRAGE DE LA BOUCLE MQTT
log_message "INFO" "Écoute des données MQTT - Broker: $BROKER:$PORT, Topic: $TOPIC"
log_message "INFO" "Appuyez sur Ctrl+C pour quitter"

mosquitto_sub -h "$BROKER" -p "$PORT" -t "$TOPIC" -v | \
while read -r line; do
  json=$(echo "$line" | cut -d' ' -f2-)

  # VÉRIFIER SI C'EST LE BON DEVICE
  appid=$(echo "$json" | jq -r '.deviceInfo.applicationId')
  devEui=$(echo "$json" | jq -r '.deviceInfo.devEui')
  [[ "$appid" != "$APPLICATION_ID" || "$devEui" != "$DEVICE_EUI" ]] && continue

  # EXTRAIRE LES DONNÉES ENCODÉES EN BASE64
  b64=$(echo "$json" | jq -r '.data')
  [[ -z "$b64" || "$b64" == "null" ]] && continue

  # DÉCODAGE ET LECTURE DES OCTETS
  read -r t_lsb t_msb q_lsb q_msb p_lsb p_msb h_lsb h_msb < <(
    echo "$b64" | base64 -d | od -An -t u1
  )

  # CONVERSION DES VALEURS
  raw_temp=$(( (t_lsb) | (t_msb << 8) ))
  raw_tds=$(( (q_lsb) | (q_msb << 8) ))
  raw_depth=$(( (p_lsb) | (p_msb << 8) ))
  raw_hum=$(( (h_lsb) | (h_msb << 8) ))

  # TEMPÉRATURE : SIGNED 16-BIT, EN CENTIÈMES DE °C
  if (( raw_temp >= 32768 )); then
    signed_temp=$(( raw_temp - 65536 ))
  else
    signed_temp=$raw_temp
  fi
  temp=$(awk "BEGIN{printf \"%.2f\", $signed_temp/100}")

  # TDS (PPM)
  tds=$raw_tds

  # PROFONDEUR : DIXIÈMES DE CM
  depth=$(awk "BEGIN{printf \"%.2f\", $raw_depth/10}")

  # HUMIDITÉ : DIXIÈMES DE %
  humidity=$(awk "BEGIN{printf \"%.2f\", $raw_hum/10}")

  # AFFICHAGE
  log_message "INFO" "Données reçues:"
  log_message "INFO" "→ Température: $temp °C"
  log_message "INFO" "→ TDS: $tds ppm"
  log_message "INFO" "→ Profondeur: $depth cm"
  log_message "INFO" "→ Humidité: $humidity %"

  # ENVOI DES DONNÉES À L'API
  send_mesure_data "${CAPTEURS["temperature"]}" "$temp"
  send_mesure_data "${CAPTEURS["turbinite"]}" "$tds"
  send_mesure_data "${CAPTEURS["niveau_eau"]}" "$depth"
  send_mesure_data "${CAPTEURS["humidite"]}" "$humidity"

  echo -e "${BLUE}-----------------------------------${NC}"
done