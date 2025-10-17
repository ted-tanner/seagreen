#!/bin/bash

sudo sysctl -w kern.aiomax=10000
sudo sysctl -w kern.aioprocmax=10000
sudo sysctl -w kern.aiothreads=12
