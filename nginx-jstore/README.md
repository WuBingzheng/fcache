## nginx jstore module

Because `fcache` does not have the ability to access the origin when request missing,
so we need the WebServer to fetch the item from origin and store it into `fcache`.

If Nginx is the WebServer, this module is used to store the missing
item by subrequest.

This is tested on Nginx 1.4 ~ 1.8.

The Nginx configuration is as follows:

    upstream origin { 
        server 192.168.0.123:8080;
    }
    upstream fcache {
        server 127.0.0.1:8535;
    }
    server {
        listen 80 default;
        proxy_intercept_errors on;
        proxy_ignore_client_abort on;
        proxy_set_header Host "$host";

        location / { 
            # if ($request_method !~ (HEAD|GET)) { return 405; }
            proxy_pass http://fcache;  # request cache. return to client, if hit
            error_page 400 404 405 500 502 504 = @pass; # jump to @pass, if cache miss(404) or fail(5xx)
        }
        location @pass {
            proxy_http_version 1.1;     # enable chunked
            proxy_pass http://origin;   # request origin
            jstore @store;              # create a subrequest to @store
        }
        location @store {
            proxy_pass http://fcache;  # store the missing item
        }
    }
