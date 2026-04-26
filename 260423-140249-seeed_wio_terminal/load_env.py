import os

Import("env")

# Read .env file and inject as build flags
env_file = os.path.join(os.getcwd(), ".env")
if os.path.exists(env_file):
    with open(env_file) as f:
        for line in f:
            line = line.strip()
            if line and not line.startswith("#"):
                key, _, value = line.partition("=")
                env.Append(CPPDEFINES=[(key.strip(), f'\\"{value.strip()}\\"')])