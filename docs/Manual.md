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
