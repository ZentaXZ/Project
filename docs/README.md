# Prompt Maestro — Gestor de Tareas y Productividad

Programa de escritorio para Windows en C con interfaz **GTK4** y control de procesos vía **WinAPI**.

## Instalación (MSYS2 en Windows)

### 1. Descargar e instalar MSYS2

Visita https://www.msys2.org/ y descarga el instalador.

### 2. Instalar dependencias (solo GTK4 — todo lo demás viene incluido)

Una vez dentro de MSYS2 MinGW64, ejecuta:

```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-gtk4 mingw-w64-x86_64-pkg-config
```

**Nota:** Este es el único paso que requiere instalación manual en tu PC. Todas las demás dependencias del proyecto (cJSON, etc.) ya están incluidas en el repositorio.

### 3. Clonar el repositorio

```bash
git clone https://github.com/ZentaXZ/Project.git
cd Project
```

### 4. Compilar

Desde la carpeta del proyecto (dentro de MSYS2 MinGW64):

```bash
make
```

Para verificar que GTK4 está disponible antes de compilar:

```bash
make setup-check
```

### 5. Ejecutar

```bash
make run
```

## Estructura del proyecto

```
Project/
├── src/                    # Código fuente
│   ├── main.c
│   ├── ui/                 # Interfaz GTK4
│   ├── tasks/              # Lógica de tareas
│   ├── stats/              # Sistema de estadísticas
│   ├── control/            # Control de procesos (WinAPI)
│   ├── notifications/      # Notificaciones y sonidos
│   └── utils/              # Utilidades (JSON, tiempo, UUIDs)
│
├── data/                   # Archivos JSON (generados en tiempo de ejecución)
│   ├── tasks.json
│   ├── daily_tasks.json
│   ├── completed_tasks.json
│   ├── stats.json
│   ├── config.json
│   ├── app_blocklist.json
│   └── app_whitelist.json
│
├── assets/                 # Iconos y sonidos
│   ├── icons/
│   └── sounds/
│
├── third_party/            # Dependencias de código fuente (versioned)
│   └── cJSON/              # Parser/generador JSON (incluido en el repo)
│       ├── cJSON.c
│       └── cJSON.h
│
├── docs/                   # Documentación
│   └── README.md
│
├── Makefile                # Build system
└── .gitignore

```

## Dependencias

### Del sistema (instalar manualmente)
- **MSYS2** (entorno de compilación)
- **GTK4** (biblioteca de interfaz gráfica)
- **pkg-config** (para ubicar librerías)
- **GCC** (compilador)

### Del proyecto (ya incluidas)
- **cJSON** (`third_party/cJSON/`) — Parser JSON, compilado junto con el resto del código
- Todos los módulos del proyecto (ui, tasks, stats, control, notifications)

## Desarrollo

### Orden de implementación de módulos (seguir estrictamente)

1. ✅ `utils/json_utils` + cJSON
2. ✅ `utils/time_utils`
3. ⏳ `tasks/task_manager`
4. ⏳ `stats/stats`
5. ⏳ `ui/ui_tasks` + `ui/ui_completed` + `ui/ui_main`
6. ⏳ `control/process_monitor`
7. ⏳ Integración final

### Compilar sin linkear (solo verificar sintaxis)

```bash
make clean
gcc -Wall -Wextra -std=c11 -c src/utils/json_utils.c -o /tmp/test.o $(pkg-config --cflags gtk4)
```

## Estado de desarrollo

Este es un proyecto en desarrollo activo. La implementación sigue el orden especificado en el spec de Prompt Maestro.

## Licencia

Pendiente de definir.
