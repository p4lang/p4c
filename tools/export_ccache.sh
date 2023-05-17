#!/bin/bash
docker cp $(docker create --rm p4c):/p4c/.ccache .
