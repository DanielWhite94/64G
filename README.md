# Building
From the root directory, run `make clean` followed by either `make release` or `make debug`.

# Sub-projects
* `client` - the game itself
    * Keys
        * Up/Down/Left/Right - move
        * Space - hold to run while moving
        * Tab - zoom out one level (or loop back around to maximum)
        * 'g' - step through grid options (initially no grid, one press gives a tile grid, and the final press also adds a coordinate grid)
* `demogen` - generates a 'demo' game with a map of the given size (including rivers and towns)
* `editor` - A GUI game editor.
* `mappng` - takes an EMap file and outputs a png image of a given size, representing a given region in the map.
* `server` - takes an SMap file and runs the game, allowing clients to connect and play.
* `slippymap` - takes an EMap and generates a series of images suitable for interactive/slippy maps (these are good for larger maps where mappng can not generate a single image with enough detail). The resulting map can be viewed via the `slippymap.html` file within the map directory.

# Examples #
![Contours](https://github.com/DanielWhite94/64G/blob/master/examples/contours.png)

![Map](https://github.com/DanielWhite94/64G/blob/master/examples/map.png)

![Political](https://github.com/DanielWhite94/64G/blob/master/examples/political.png)

![Temperature](https://github.com/DanielWhite94/64G/blob/master/examples/temperature.png)

![Town](https://github.com/DanielWhite94/64G/blob/master/examples/town.png)
