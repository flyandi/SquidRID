## Serial Protocol

SquidRID exposes a command and event pattern via Serial that allows external configuration as well receiving status updates. 

All commands and events are case sensitive and the default baud rate is `115200` bps. 

### Supported Commands


| Request | Description           | Event | Example    |
| ------- | --------------------- | ----- | ---------- |
| `$V`    | Requests the version  | `$V   <VERSION>` | `$V | 1000` |
