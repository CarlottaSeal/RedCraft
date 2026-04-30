# RedCraft

A Minecraft-style voxel game built from scratch in C++ on a custom engine. Procedural world generation, redstone circuits, farming, dynamic weather, and chunk streaming with multithreaded generation.

Demo & more details: https://carlottak16mars.wixsite.com/seal/blank

## World Generation

Terrain is generated through a multi-stage pipeline:

1. **Biome sampling** — 5 noise layers (temperature, humidity, continentalness, erosion, weirdness) mapped to a biome lookup table. Produces plains, forests, taiga, jungle, savanna, desert, snowy biomes, and more.
2. **3D density field** — 8-octave Perlin noise with a Z-bias curve that shapes terrain height. Continental noise (scale 1024) controls land vs ocean at the macro level.
3. **Cave carving** — Three distinct cave types:
   - **Cheese caves**: large voids from 3D noise (threshold 0.65)
   - **Spaghetti caves**: tunnel networks from dual 2D noise planes (threshold 0.18)
   - **Noodle caves**: thin passages from ridge noise (threshold 0.08)
   - Depth-aware: caves become rarer near the surface
4. **Surface building** — Biome-specific block layers (grass/dirt, sand, snow, etc.)
5. **Feature placement** — 6 tree species (oak, birch, spruce, jungle, acacia, dark oak) with 12+ size variants. Stamp-based placement for efficiency. Cactus and sugar cane in appropriate biomes.

## Chunk System

Each chunk is 16x16x128 blocks (32,768 total). Block indices use bit-shifted coordinates (4+4+7 bits) for fast lookup without division.

- Activation radius: 320 units around the player
- Up to 8 concurrent generation jobs on worker threads
- RLE-compressed binary serialization for save/load ("GCHK" format)
- 3 bytes per block: type index + flags + packed light (4-bit outdoor + 4-bit indoor)

## Block & Light System

113 block types defined via XML. Each block has visibility, solidity, opacity flags, per-face sprite UVs, and light emission values. Light propagation runs per-chunk with dirty-flag optimization.

## Redstone

Tick-based redstone simulation with 15-level power propagation and per-block attenuation.

Components: wire (dot/line/corner/cross), torches, repeaters (4-tick delay), observers, pistons, sticky pistons, levers, buttons, lamps, redstone blocks. Scheduled update queue capped at 1000 updates/tick. Burnout cleanup runs every 10 seconds.

## Farming

Crop system tracking up to 1000 active plots. Growth stages: wheat (8), carrots (4), potatoes (4), beetroots (4). Sugar cane, cactus, nether warts, pumpkins, melons. Farmland tracks wet/dry state based on water proximity. Physics-enabled item drops with collection radius.

## Weather

12 weather states: clear, cloudy, overcast, light/medium/heavy rain, light/medium/heavy snow, thunderstorm, blizzard, fog. Smooth interpolation between states. Wind direction and strength driven by 3-octave Perlin noise. Lightning frequency scales with storm intensity (3-15s intervals). Procedural cloud mesh rebuilt dynamically with 8-octave noise and coverage control.

## Inventory & Crafting

9-slot hotbar + 27-slot main inventory. 3x3 crafting grid. Furnace smelting. Chest storage with physics-based entity system.

## Player & Physics

AABB collision with step-up mechanics. First-person, third-person, and overhead camera modes. Run speed 12 m/s, max 20 m/s. Keyboard + Xbox controller input. Block raycast for placement and breaking.

## Rendering

Custom DX11 renderer with per-block face culling, distance fog, dual indoor/outdoor lighting, shadow mapping, and multiple texture pack support (Faithful, Dokucraft, Classic).

## UI

Main menu, world select with multiple save slots, pause menu, settings screen, inventory, crafting, furnace, chest, farm monitor, and redstone configuration screens. HUD with hotbar, crosshair, and debug overlay (F1).

## Build

Built on the SD Engine (custom C++ engine with DX11 renderer, job system, audio, input, math, ImGui debug tools).

```
Engine:  C++ / DX11 / HLSL
Source:  ~50 .cpp/.hpp files across Game, Generator, Gameplay, Physics, Weather, UI
```
