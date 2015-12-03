#!/bin/sh
KEY=`cat ~/.ssh/id_rsa.pub`
echo "Adding public RSA key to $1"
ssh root@$1 "echo '$KEY' >> /etc/dropbear/authorized_keys"
