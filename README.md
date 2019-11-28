# SpatialOS C project

## Dependencies

This project contains workers written in both C and C++ and use [CMake](https://cmake.org/download/)
as their build system. Your system needs to satisfy the
[C++ prerequisites](https://docs.improbable.io/reference/latest/cppsdk/setting-up#prerequisites).
In practice, this just means having a fairly recent compiler that supports C++11 or later.

## Quick start

Build the project and start it with the default launch configuration:

```
spatial worker build --target windows
spatial local launch
```

(Replacing `windows` with `macos` on macOS, or `linux` on Linux).

This will launch SpatialOS locally with a single C++ "physics" worker that updates the position of
a single entity. You may also see a 2nd entity called "physics-worker" created. This entity
represents the managed worker itself.

Note: If you run `spatial worker build` without a `--target` parameter (or with the wrong target
parameter), then the CMake cache for each worker (`workers/<worker>/cmake_build`) may end up in
a corrupt state. To recover, just run `spatial worker clean` to delete the CMake caches.

Now, you can connect the C client worker. The worker can be launched with the following command:

* Client: `spatial local worker launch client local`

## Scenario

Small (unsecure) Login process just to get used to the C SDK that at the moment only supports one client. 
- Client requests LoginManager component owner entity from Server.
- Client triggers Login command onto LoginManager component.
- Server validates Login.
	- Success.
		- Request entity creation to SpatialOS with a Client owned Player Component.
		- Success respond command to Client.
	- Failure.
		- Failure respond command to Client.
- Server receives SpatialOS entity created callback.
	- Server triggers Init command onto Player component that is received by the authored Client. 
	- Client receives command and keeps the EntityId of its Player.

## Snapshot

The snapshot exists in both json and binary format in the `snapshots` folder. There is no script
to generate the snapshot as the snapshot was written by hand in the json format, but it's possible
to make simple changes to the json format and regenerate the binary snapshot from it. To update the
binary snapshot after making a change, run the following command:

```
spatial project history snapshot convert --input-format=text --input=snapshots/default.json --output-format=binary --output=snapshots/default.snapshot
```

To verify the binary snapshot after making a change, run the following command:

```
spatial project history snapshot convert --input-format=binary --input=snapshots/default.snapshot --output-format=text --output=snapshots/default.txt
```
