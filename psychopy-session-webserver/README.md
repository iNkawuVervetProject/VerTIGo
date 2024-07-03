# psychopy-session-webserver

A package that allows to run and control a session (i.e. multiple experiment with multiple participant) via a REST API.

## Fetaures

 * Automatic detection of experiments and needed resource files (image, sound,
   movies) via filesystem notifications.
 * List of participant and session run saved in `XDG_DATA_HOME`.
 * Server Side Event of the session state for easily keeping the frontend in
   sync with the session server.



## Installing

Install this package with the following shell command:

```bash
pip install psychopy-session-webserver
```


## Usage

```bash
python -m psychopy_session_webserver [options] sessionDir
```

### Common options

* `sessionDir` : a directory where the '.psyexp' and their relative resource
  files for the session could be found.
* `-L/--listen`: IP addresses or subnet CIDR subnet notation to listen
  too. Default to private subnet ( 192.168.0.0/16, 10.0.0.0/8, 100.64.0.0/10)
* `-p / --port`: Port to listen to ( default: 5000)
* `--data-dir`: Directory to save the sessions data, defaults to
  `sessionDir/data` if not specified.

## Systemd service

An examples for a systemd service can be found in
`misc/psychopy-session-webserver.service`. Since psychopy wants to open a
window, it is easier to set it up as user service.

```bash
git clone https://github.com/atuleu/zaci
cd zaci/psychopy-session-webserver

mkdir -p $HOME/.config/systemd/user
mkdir -p $HOME/.local/bin
python -m venv $HOME/psychopy/venv
source $HOME/psychopy/venv/bin/activate
pip install psychopy-session-webserver
deactivate

cp misc/psychopy-session-webserver.service $HOME/.config/systemd/user
cp misc/psychopy-session-webserver.sh $HOME/.local/bin

systemctl --user enable psychopy-session-webserver.service
systemctl --user start psychopy-session-webserver.service
```
