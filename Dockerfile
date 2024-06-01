# Utiliser une image Python comme base
FROM python:3.8-slim-buster

ENV FLASK_APP=main.py

# Définir le répertoire de travail dans l'image
WORKDIR /app

# Copier les dépendances
COPY requirements.txt requirements.txt

# Installer les dépendances
RUN pip install -r requirements.txt

RUN mkdir -p db

# Copier le code de l'application
COPY . .

# Exposer le port 5000 pour l'application Flask
EXPOSE 5000

# Définir la commande pour démarrer l'application
CMD ["flask", "run", "--host=0.0.0.0"]
