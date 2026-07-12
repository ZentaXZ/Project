# Prompt Maestro — Gestor de Tareas y Productividad

Programa de escritorio para Windows en C con interfaz GTK3 y control de procesos vía WinAPI.

## Instalación (MSYS2 en Windows)

### 1. Descargar e instalar MSYS2

https://www.msys2.org/

### 2. Instalar dependencias

```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-gtk3 mingw-w64-x86_64-pkg-config
```

### 3. Descargar cJSON

```bash
cd third_party
git clone https://github.com/DaveGamble/cJSON.git
cd cJSON
```

### 4. Compilar

```bash
make
```

### 5. Ejecutar

```bash
make run
```

## Estructura del proyecto

- `src/` — Código fuente
- `data/` — Archivos JSON de configuración y almacenamiento
- `assets/` — Iconos y sonidos
- `third_party/` — Librerías externas (cJSON)
- `docs/` — Documentación

## Estado de desarrollo

Este es un proyecto en desarrollo temprano.
