# Manual

This is a step-by-step guide to introduce you to various aspects of the engine. More specific documentation can be always found in the class/function/variable documentation in the source code.

# Creating a new project

TODO

# Game instance and game window

`ge_game_instance` is the main object of your application, it manages main subsystems like the renderer, game worlds and etc. After you create a new project your first step should be to create a new class derived from `ge_game_instance` - this will be your game's instance.

In the constructor of your game instance you usually create a game window (to display game world) and a render target (to render the game world onto), then assign that render target to the window (telling the window to display the contents of that render target).

`ge_game_instance` provides `create_window...` functions which you use to create new windows. Windows are used to display contents of render targets (basically textures) and cameras are used to draw on render targets.

Even if you haven't created a single window (headless mode) to start your game you need to call the function `ge_game_instance::run_game_loop` which (when entered) calls the first virtual function of game instance: `on_game_started` - you override this function to do game start functionality of your game.

In order to stop game loop you should use `ge_game_instance::stop_game_loop`, this will mark a flag to stop game loop and the `run_game_loop` function will exit. Before leaving game loop another virtual function of game instance will be called: `on_game_finished` - this is where you do game destruction/cleanup logic.

After the game has started `on_tick` (virtual function of game instance) will be called every frame.

# Game world and nodes

After your game has started you need to create a new world. This is done using world manager:

```Cpp
void
my_game_instance::on_game_started() {
    // ... some code ...

    ge_world* world = get_world_manager()->create_world();
}
```

By default a world only has 1 node - root node.

The engine uses nodes for game entities, this is similar to Godot's node system. You have a base class `ge_node` from which all nodes derive. This base class implements hierarchy functionality: allows attaching node to some (other) parent node. There are different kinds of nodes such as: `ge_spatial_node` (a node that implements positioning in 3D space, has properties like location, rotation and scale), `ge_mesh_node` (a node that derives from spatial node and implements 3D model rendering) and so on.

Because world always has a valid spawned root node you spawn new game entities by attaching newly created nodes to already spawned ones, this makes the newly attached nodes to be spawned. This means that world operates only on spawned nodes.

If you need a custom game entity you create a new node class derived from some of the existing node classes.

# Memory leak checks

The engine has a silly little memory leak checker that you can use on Windows, see contents of the file `src/engine_lib/mem_leak_check.hpp` and how it's used in the editor's `main.cpp`. If any memory leaks occurred after closing the game you will see a report about memory leaks in the log and in the Visual Studio's output tab (debug category).

# Headless mode

In case you want to run a game server, possibly on a Linux machine, all you need to do is to not create any windows and specify a non-zero tickrate to `run_game_loop`.
