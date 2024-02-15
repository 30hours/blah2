# todo: currently not working

## Usage

Install a recent `nodejs` using [nvm](https://github.com/nvm-sh/nvm).

```
curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.39.7/install.sh | bash
nvm install node 21.6.2
```

Install the latest [devcontainer CLI](https://code.visualstudio.com/docs/devcontainers/devcontainer-cli).

```
npm install -g @devcontainers/cli
devcontainer --version
```

Run the devcontainer.

```
devcontainer up --workspace-folder .
```