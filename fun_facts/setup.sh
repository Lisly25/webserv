#!/bin/bash

set -e

if ! command -v python3 &> /dev/null; then
    echo "Python3 not found. Installing Python3..."
    sudo apt update
    sudo apt install -y python3 python3-venv python3-pip
fi

if ! command -v pip3 &> /dev/null; then
    echo "pip3 not found. Installing pip3..."
    sudo apt update
    sudo apt install -y python3-pip
fi

if [ ! -d "venv" ]; then
    echo "Creating virtual environment..."
    python3 -m venv venv
fi

source venv/bin/activate

pip install --upgrade pip

pip install Flask

echo "Starting the Flask app..."

export FLASK_APP=app.py
export FLASK_RUN_HOST=0.0.0.0
export FLASK_RUN_PORT=8085

flask run
