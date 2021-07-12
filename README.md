# DiskArray

A module to save part of array, can be immensely helpful to reduce RAM usage of simulations (**specially if you are memory-bound instead of CPU-bound, in latter case you should not need this**) that keep track of past events, though will take more time to load old entries.

**Will use** protocol buffers for the serialisation and deserialisation parts
And the part that our small library does is decide when and what elements to 'store to disk and deallocate' (currently consecutive elements from the first part of array), abstract random access to elements for both in-memory and on-disk elements, and for this the indexing (keeps a .index file to provide that random access)

> It was a module in [worldlinesim](https://github.com/adi-g15/worldlinesim), and has been separated into a different lib and in process of a revamp here.

# Dependencies

* ProtoBuff ([Protocol Buffers](https://developers.google.com/protocol-buffers/))

## Possible Usecase

In worldline simulator, it keep tracks of all past events in all worlds, that is multiple large arrays, though we almost never access these, do we ? It was used for 'time travelling' to past, which is rare event, so making it bit more costly, which making RAM available for other tasks.
And RAM should not be wasted such, so it saves each array to it's own file, though you still have random access like an array, though with added penalty for old entries.

