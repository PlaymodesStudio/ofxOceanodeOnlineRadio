#!/usr/bin/env python3
import requests
import json
import os
import socket
from urllib.parse import urlparse
from random import choice

def get_working_api_url():
    """Get a working radio-browser API server through DNS lookup"""
    try:
        # Get IP addresses through DNS lookup
        ips = socket.getaddrinfo('all.api.radio-browser.info', 443, proto=socket.IPPROTO_TCP)
        
        # Test each server
        servers = [
            f"de1.api.radio-browser.info",
            f"fr1.api.radio-browser.info",
            f"nl1.api.radio-browser.info"
        ]
        
        # Try each server and return the first working one
        for server in servers:
            try:
                test_url = f"https://{server}/json/stations/topclick/100"
                response = requests.get(test_url, timeout=5)
                if response.status_code == 200:
                    print(f"Using server: {server}")
                    return server
            except:
                continue
                
        return None
    except Exception as e:
        print(f"Error during DNS lookup: {e}")
        return None

def download_radio_list(base_url):
    """Download radio station list from URL"""
    try:
        headers = {
            'User-Agent': 'RadioPlayer/1.0',
            'Content-Type': 'application/json'
        }
        
        # Get most popular stations first
        url = f"https://{base_url}/json/stations/topclick/100"
        print(f"Downloading from: {url}")
        
        response = requests.get(url, headers=headers, timeout=10)
        response.raise_for_status()
        stations = response.json()
        
        # Get some recently clicked stations too
        url = f"https://{base_url}/json/stations/lastclick/100"
        print(f"Downloading more stations from: {url}")
        
        response = requests.get(url, headers=headers, timeout=10)
        response.raise_for_status()
        stations.extend(response.json())
        
        return stations
    except Exception as e:
        print(f"Error downloading radio list: {e}")
        return None

def is_valid_stream_url(url):
    """Basic validation of stream URL"""
    try:
        parsed = urlparse(url)
        return all([parsed.scheme, parsed.netloc]) and any(ext in url.lower() for ext in 
            ['.mp3', '.m3u', '.m3u8', '.pls', '.aac', 'stream', 'listen'])
    except:
        return False

def import_radio_list(stations_file):
    """Import radio stations"""
    # Get working API server
    base_url = get_working_api_url()
    if not base_url:
        print("Could not find a working radio-browser server")
        return
    
    # Download stations
    stations = download_radio_list(base_url)
    if not stations:
        print("Failed to download stations")
        return
    
    # Load existing stations
    existing_stations = {}
    if os.path.exists(stations_file):
        with open(stations_file, 'r') as f:
            existing_stations = json.load(f)
    
    # Process and categorize stations
    new_stations = {}
    print(f"Processing {len(stations)} stations...")
    
    for station in stations:
        if all(key in station for key in ['name', 'url', 'tags']):
            # Clean and validate the URL
            url = station['url']
            if not is_valid_stream_url(url):
                continue
            
            # Determine category from tags
            tags = station['tags'].lower() if station['tags'] else ''
            category = 'other'
            for tag in ['jazz', 'rock', 'classical', 'pop', 'news', 'electronic', 'ambient', 
                       'folk', 'metal', 'blues', 'country', 'reggae', 'world']:
                if tag in tags:
                    category = tag
                    break
            
            # Clean station name
            name = station['name'].lower()
            name = ''.join(c for c in name if c.isalnum() or c in '-_ ')
            name = name.replace(' ', '_')[:30]  # Limit name length
            
            station_id = f"{category}.{name}"
            new_stations[station_id] = url
    
    # Merge with existing stations
    merged_stations = {**existing_stations, **new_stations}
    
    # Save merged stations
    with open(stations_file, 'w') as f:
        json.dump(merged_stations, f, indent=2, sort_keys=True)
    
    print(f"Added {len(new_stations)} new stations")
    print(f"Total stations: {len(merged_stations)}")
    return merged_stations

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    stations_file = os.path.join(script_dir, 'stations.json')
    
    print("Radio Station Importer")
    print("This will download and import popular radio stations.")
    print("Your existing stations will be preserved.")
    
    input("\nPress Enter to continue...")
    
    stations = import_radio_list(stations_file)
    
    if stations:
        print("\nCategories imported:")
        categories = {}
        for station in stations.keys():
            category = station.split('.')[0]
            categories[category] = categories.get(category, 0) + 1
        
        for category, count in sorted(categories.items()):
            print(f"{category}: {count} stations")

if __name__ == "__main__":
    main()