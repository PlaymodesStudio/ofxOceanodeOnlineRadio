#!/usr/bin/env python3
import requests
import json
import os
import socket
from urllib.parse import urlparse

def get_working_api_url():
    """Obté un servidor API de Radio Browser funcionant mitjançant una comprovació dels servidors disponibles."""
    servers = [
        "de1.api.radio-browser.info",
        "fr1.api.radio-browser.info",
        "nl1.api.radio-browser.info",
        "us1.api.radio-browser.info",
        "gb1.api.radio-browser.info"
    ]
    
    for server in servers:
        try:
            test_url = f"https://{server}/json/stations/topclick/100"
            response = requests.get(test_url, timeout=5)
            if response.status_code == 200:
                print(f"Usant el servidor: {server}")
                return server
        except requests.RequestException:
            continue
    
    print("No s'ha pogut trobar un servidor API funcionant.")
    return None

def download_techno_stations(base_url):
    """Descarrega la llista d'estacions de ràdio categorizades com a 'techno'."""
    try:
        headers = {
            'User-Agent': 'RadioPlayer/1.0',
            'Content-Type': 'application/json'
        }
        
        # Buscar estacions amb la etiqueta 'techno'
        url = f"https://{base_url}/json/stations/bytag/techno"
        print(f"Descarregant estacions des de: {url}")
        
        response = requests.get(url, headers=headers, timeout=10)
        response.raise_for_status()
        stations = response.json()
        
        print(f"S'han descarregat {len(stations)} estacions de techno.")
        return stations
    except Exception as e:
        print(f"Error descarregant les estacions de techno: {e}")
        return None

def is_valid_stream_url(url):
    """Validació bàsica de l'URL de la transmissió."""
    try:
        parsed = urlparse(url)
        return all([parsed.scheme, parsed.netloc]) and any(ext in url.lower() for ext in 
            ['.mp3', '.m3u', '.m3u8', '.pls', '.aac', 'stream', 'listen'])
    except:
        return False

def import_techno_stations(stations_file):
    """Importa les estacions de ràdio de techno i les desa al fitxer JSON."""
    # Obtenir el servidor API funcionant
    base_url = get_working_api_url()
    if not base_url:
        print("No s'ha pogut trobar un servidor API de Radio Browser funcionant.")
        return
    
    # Descarregar les estacions de techno
    stations = download_techno_stations(base_url)
    if not stations:
        print("Ha fallat la descàrrega de les estacions de techno.")
        return
    
    # Carregar les estacions existents si el fitxer ja existeix
    existing_stations = {}
    if os.path.exists(stations_file):
        with open(stations_file, 'r', encoding='utf-8') as f:
            existing_stations = json.load(f)
    
    # Processar i validar les estacions
    new_stations = {}
    print(f"Processant {len(stations)} estacions de techno...")
    
    for station in stations:
        if all(key in station for key in ['name', 'url', 'tags']):
            # Validar l'URL de transmissió
            url = station['url']
            if not is_valid_stream_url(url):
                continue
            
            # Assignar la categoria 'techno'
            category = 'techno'
            
            # Netejar el nom de l'estació
            name = station['name'].strip().lower()
            name = ''.join(c for c in name if c.isalnum() or c in '-_ ')
            name = name.replace(' ', '_')[:30]  # Limitar la longitud del nom
            
            station_id = f"{category}.{name}"
            new_stations[station_id] = url
    
    # Fusionar amb les estacions existents
    merged_stations = {**existing_stations, **new_stations}
    
    # Guardar les estacions en el fitxer JSON
    with open(stations_file, 'w', encoding='utf-8') as f:
        json.dump(merged_stations, f, indent=2, sort_keys=True, ensure_ascii=False)
    
    print(f"S'han afegit {len(new_stations)} noves estacions de techno.")
    print(f"Total d'estacions de techno: {len(merged_stations)}")
    return merged_stations

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    stations_file = os.path.join(script_dir, 'techno.json')
    
    print("Importador d'Estacions de Ràdio de Techno")
    print("Aquest script descarregarà i importarà estacions de ràdio de techno populars.")
    print("Les estacions existents seran preservades.")
    
    input("\nPrem Enter per continuar...")
    
    stations = import_techno_stations(stations_file)
    
    if stations:
        print("\nEstacions de Techno Importades:")
        print(f"Total estacions de techno: {len(stations)}")
        # Opcional: Llistar algunes de les estacions importades
        for station_id, url in list(stations.items())[:10]:  # Mostrar les primeres 10
            print(f"{station_id}: {url}")

if __name__ == "__main__":
    main()
