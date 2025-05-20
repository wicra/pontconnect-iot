# Guide de configuration ChirpStack pour PONTCONNECT

Ce guide vous explique comment configurer le serveur ChirpStack pour recevoir et décoder les données de votre dispositif PONTCONNECT.

## Table des matières

- [Guide de configuration ChirpStack pour PONTCONNECT](#guide-de-configuration-chirpstack-pour-pontconnect)
  - [Table des matières](#table-des-matières)
  - [Connexion à ChirpStack](#connexion-à-chirpstack)
  - [Configuration de la passerelle (Gateway)](#configuration-de-la-passerelle-gateway)
  - [Configuration du Wi-Fi](#configuration-du-wi-fi)
  - [Configuration de l'application](#configuration-de-lapplication)
  - [Configuration du service LoRa](#configuration-du-service-lora)
  - [Enregistrement de l'appareil](#enregistrement-de-lappareil)

## Connexion à ChirpStack

1. Ouvrez votre navigateur et accédez à l'adresse IP de votre serveur ChirpStack
2. Utilisez vos identifiants pour vous connecter

![Page de connexion ChirpStack](./page-chirpstack-connexion.png)

Après la connexion, vous verrez le tableau de bord principal :

![Tableau de bord sans configuration](./page-chirpstack-dashboard-no-config.png)

## Configuration de la passerelle (Gateway)

Vous devez d'abord configurer une passerelle LoRaWAN pour recevoir les données :

1. Dans le menu, accédez à "Gateways"
2. Cliquez sur "Add Gateway"
3. Remplissez les informations requises :
   - Gateway ID : identifiant unique de votre passerelle
   - Description : description de la passerelle
   - Network Server : sélectionnez votre serveur réseau
   - Location : définissez l'emplacement de la passerelle sur la carte

![Configuration de la passerelle](./page-chirpstack-gateways-config.png)

## Configuration du Wi-Fi

Si vous utilisez une passerelle avec Wi-Fi, configurez-la comme suit :

1. Accédez à la page de configuration de votre passerelle
2. Allez dans les paramètres Wi-Fi
3. Scannez les réseaux disponibles

![Scan Wi-Fi](./page-wi-fi-scan.png)

4. Sélectionnez votre réseau et entrez vos identifiants

![Configuration Wi-Fi](./page-wi-fi-config.png)

5. Une fois connecté, vérifiez l'état de la connexion

![Wi-Fi connecté](./page-wi-fi-connected.png)

## Configuration de l'application

Pour gérer vos appareils PONTCONNECT :

1. Retournez au tableau de bord ChirpStack
2. Accédez à "Applications"
3. Cliquez sur "Add Application"
4. Renseignez :
   - Nom de l'application (ex : "PONTCONNECT")
   - Description (ex : "Surveillance aquatique")
   - Service profile : sélectionnez un profil adapté

## Configuration du service LoRa

Dans la section LoRa de votre passerelle :

1. Accédez à la page "Forwarder"
2. Configurez l'adresse de votre serveur ChirpStack

![Configuration LoRa](./page-lora-config.png)

## Enregistrement de l'appareil

Pour chaque dispositif PONTCONNECT :

1. Dans votre application, cliquez sur "Devices"
2. Cliquez sur "Add device"
3. Entrez :
   - Device EUI : identifiant unique du dispositif (visible dans le code)
   - Device name : nom de votre dispositif
   - Device profile : sélectionnez un profil adapté
4. Sur la page suivante, configurez les clés de sécurité :
   - Application key : clé utilisée pour chiffrer les communications


&copy; 2025 PONTCONNECT - Guide de configuration ChirpStack
