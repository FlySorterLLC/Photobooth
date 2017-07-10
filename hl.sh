#!/bin/bash

if [ -f images/Upper000.png ]; then
    LASTNUM=`ls images/Upper*.png | tail -n1 | cut -d'/' -f 2 | cut -d'r' -f 2 | cut -d'.' -f 1`

    NP=`echo $LASTNUM + 1 | bc`

    echo "Last image was $LASTNUM, starting at $NP"

  else

    echo "Starting at 0."

    NP=0

fi

./HandLoad $NP
