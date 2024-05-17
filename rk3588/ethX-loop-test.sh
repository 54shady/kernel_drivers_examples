#!/usr/bin/env bash

# https://superuser.com/questions/1568469/ping-one-interface-from-the-other-same-host

# define two ethernet interface and namespace
dev1=eth0
dev2=eth1
netns=ns1

# Create a new network namespace
ip netns add "$netns"

# Bring the devices down in the default namespace.
ip link set dev "$dev1" down
ip link set dev "$dev2" down

# Add one of the devices (dev2)to the new namespace (it will disappear from the default namespace)
ip link set dev "$dev2" netns "$netns"

# Assign IP addresses.
ip address add 192.168.10.2/24 dev "$dev1"
ip netns exec "$netns" ip address add 192.168.10.3/24 dev "$dev2"

# Bring the interfaces up.
# The namespace contains its own loopback device lo, just in case, because in general programs may want to rely on it.
ip link set dev "$dev1" up
ip netns exec "$netns" ip link set dev "$dev2" up
ip netns exec "$netns" ip link set dev lo up

# Basic cleanup: delete the network namespace.
#ip netns del "$netns"
