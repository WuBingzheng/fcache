## fcache

`fcache` is a lightweight, reliable and high-performance HTTP cache,
based on file, suitable for CDN especially.

`fcache` is a bit like `memcache`, with 2 main difference:

* HTTP protocol, more compatible with HTTP proxy and server;
* based on file, larger space.

Compared to other HTTP caches (e.g. `Squid` and `Varnish`), `fcache`
is much more lightweight and easy to use.


## configuration file

See `fcache.conf.sample`.

Only `device` and `listen` are required.

Use `device` to assign some files for storage. Normal files and block device files
are supported, while block device (raw disk or disk patition) is recommended in 
production for better performance.

Use `listen` to assign some services. You can assign multiple services, with
difference configuration and for independent statistics. For example, one for pictures,
one for static files, and one for htmls.

Other commands are optional, and their default values are given in `fcache.conf.sample`.

All the configuration can be updated by reload, without restarting.


## usage

Build:

    make   # "output/" will be created

After creating a simple configure file, start up:

    ./fcache [options]   # "fcache.pid" will be created

Quit:

    echo quit | nc 127.1 5210

Reload configure:

    echo reload | nc 127.1 5210

Running status:

    echo status | nc 127.1 5210

where `5210` is the default admin port.


## works with Nginx

Because `fcache` does not have the ability to access the origin when request missing,
so we need the WebServer to fetch the item from origin and store it into `fcache`.

If Nginx is the WebServer, a filter module `jstore` is used to store the missing
item by subrequest.

See `nginx-jstore\` for the code and usage.


## OS-machine

GNU/Linux on x86_64


## licence

GPLv2
