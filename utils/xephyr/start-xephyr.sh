#!/bin/bash
#
# usage: ./start-xephyr.sh &
#
# see xephyr-notion.sh for more

display=${NOTION_TEST_DISPLAY:-:1.0}

# i.e. (1920x1080)/4
w=${NOTION_TEST_WIDTH:-960}
h=${NOTION_TEST_HEIGHT:-540}

Xephyr $display -ac -br -noreset -screen ${w}x${h}
