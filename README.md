# ArrayDiskSaver

A module to save part of array (initial indices to disk), can be immensely helpful to reduce RAM usage of simulations that keep track of past events, though will take more time to load old entries.
> It was a module in [worldlinesim](https://github.com/adi-g15/worldlinesim), and has been separated into a different lib and in process of a revamp here.

## Possible Usecase

In worldline simulator, it keep tracks of all past events in all worlds, that is multiple large arrays, though we almost never access these, do we ? It was used for 'time travelling' to past, which is rare event, so making it bit more costly, which making RAM available for other tasks.
And RAM should not be wasted such, so it saves each array to it's own file, though you still have random access like an array, though with added penalty for old entries.

