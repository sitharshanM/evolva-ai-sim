# Digital Life Simulator 🧬

A real-time evolutionary simulation where **thousands of virtual organisms** evolve neural-network-driven behaviors across generations — guided by an **Ollama LLM World God** that shapes the environment every 20 seconds.

## What You'll See

- Organisms (colored triangles) navigating a 3000×3000 world
- Green dots = food; organisms race to eat it
- Predators (dark, aggressive) hunting herbivores (bright, evasive)
- Population graphs tracking herbivore/carnivore ratio over time
- Every ~20 seconds: Ollama issues a **divine decree** (plague, food surge, predator wave...) 
- The LLM's narration appears as a toast at the top of the screen

## Architecture

```
Ollama LLM (llama3.1)          ← World God, calls every 20 sec
        ↓ decrees
World Simulation               ← Toroidal 2D, spatial hashing
        ↓ per-tick
Organism × 400–2000             ← Each has DNA + neural network
  DNA (707 float genes)
  NeuralNet (GRU memory, 700 weights)
  Sensors: food/enemy/ally vision, hunger, energy
  Outputs: move, turn, attack, reproduce
        ↓ selection pressure
Evolution (continuous)          ← No discrete generations, live birth/death
```

## Build Requirements

- **Windows 10/11**
- **Visual Studio 2022** (Community is fine) — includes CMake and compiler
- **Git** (for CMake FetchContent to download deps)
- **Ollama** running locally (`ollama serve`)
- **llama3.1** pulled (`ollama pull llama3.1`)

## Build & Run

```bat
build.bat
```

First build downloads: GLFW, GLM, ImGui, nlohmann/json, GLAD (~50MB, cached).

Then run:
```
build\bin\Release\DigitalLife.exe
```

## Controls

| Key / Mouse | Action |
|-------------|--------|
| Left Click  | Select organism (shows inspector) |
| Right Drag  | Pan camera |
| Scroll Wheel | Zoom in/out |
| Space       | Pause / Resume |
| G           | Force immediate Ollama decree |
| R           | Reset simulation |
| +  /  -     | Speed up / slow down (1x–20x) |

## LLM Decrees (World God)

Every 20 seconds, the simulation state is sent to `llama3.1`:
- Population counts, herbivore %, carnivore %
- Average speed, vision, aggression, metabolism
- Generation count, species count

The LLM responds with a JSON decree from these options:
- `food_surge` — spawn hundreds of food particles
- `food_famine` — remove food, starve the population
- `plague` — random energy drain over time  
- `predator_wave` — spawn powerful carnivore invaders
- `population_cull` — kill the weakest 20%
- `genetic_drift_boost` — triple mutation rate
- `resource_cluster` — concentrate food in one area
- `temperature_shift` — change global metabolism multiplier
- `storm` — scatter organisms in an area

## Emergent Behaviors to Watch

- **Herbivore/carnivore split** — the population naturally bifurcates
- **Speed arms race** — predator speed vs prey speed co-evolves
- **Flocking** — organisms cluster near allies for protection
- **Predator-prey cycles** — Lotka-Volterra dynamics emerge naturally
- **Memory exploitation** — GRU memory lets organisms learn patrol routes
- **Niche specialization** — after many generations, diverse strategies appear
