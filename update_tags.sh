#!/bin/bash

find . -type f -name "*.[ch]" -print | etags -
