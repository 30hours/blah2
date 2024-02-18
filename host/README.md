# blah2 Host

A reverse proxy to host blah2 on the internet.

## Description

This can be used to forward the radar to the internet. The radar front-end is at `localhost:49152`, and the API is located at `localhost:3000/api/<data>`. This reverse proxy forwards `localhost:49152` to a port of your choosing, and forwards `localhost:3000` to the same port at `/api/`.

## Usage

**docker-compose.yml**

- Change the output port from `8080` as desired in `docker-compose.yml`.
- The environment variable `VIRTUAL_HOST=domain.tld` is only applicable if also using [jwilder/nginx](https://github.com/nginx-proxy/nginx-proxy).
- If using [jwilder/nginx](https://github.com/nginx-proxy/nginx-proxy), ensure both containers are on the same network with `sudo docker network create <name>`. Otherwise the network configuration can be deleted.

**nginx.conf**

- Edit the `backend_ip` and `domain_name` variables in `nginx.conf`.