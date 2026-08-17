StrixVerse - client
===================

To play:

  1. Run StrixVerseClient.exe
  2. Create an account, or sign in if you already have one

Connecting to a friend's server
-------------------------------
Open configs\client.json and set the host to the machine running the server:

    "server":
    {
        "host": "192.168.0.195",
        "port": 17091
    }

"127.0.0.1" means "this same computer", so it only works if you are running
the server yourself. Everyone else needs the host's address.

The host must allow inbound TCP on port 17091 through their firewall. On the
same home network the address above is enough; over the internet the host also
needs to forward that port on their router.

Display
-------
Window size, fullscreen and vsync are in the same file. The interface is laid
out on a 1920x1080 canvas and scales to any window size, so a smaller or
larger screen is fine.

Nothing here needs installing, and no Visual C++ redistributable is required.
