# Use a lightweight Python base image
FROM python:3.9-slim-bullseye

# Set the working directory in the container
WORKDIR /app

# Install system dependencies if any (e.g., for matplotlib)
# For matplotlib, some system libraries might be needed for certain backends.
# 'pkg-config' and 'libfreetype6-dev', 'libpng-dev' are common for image processing.
RUN apt-get update && apt-get install -y \
    pkg-config \
    libfreetype6-dev \
    libpng-dev \
    && rm -rf /var/lib/apt/lists/*

# Copy the requirements file and install Python dependencies
# This step is optimized for Docker caching: if requirements don't change,
# this layer won't be rebuilt.
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copy the visualization script into the container
COPY visualize_mbo.py .

# Set the entrypoint for the container
# This makes the script executable directly when the container runs
ENTRYPOINT ["timeout", "300s", "python", "visualize_mbo.py"]

