#!/usr/bin/env zsh
#
# Example:
# rofi -show notion -modi "notion:'./notion_rofi.sh focuslist'" -scroll-method 1

function escape {
    sed "s/'/\\\'/g"
}

MENU_NAME=$1

if [[ -z $2 ]]; then
    # No argument -> generate menu entries

    # Unfortunately there's no good way getting pure stdout from notionflux?
    # Massage the format of the return string into a usable list.

    notionflux -e "return rofi.menu_list('$MENU_NAME')"  \
    | sed -e 's/\\"/"/g' -e 's/^"//' -e 's/\\$//g' -e '$d'
else
    # Entry selected

    ENTRY=$(echo $2 | escape)
    notionflux -e "rofi.menu_action('$MENU_NAME', '$ENTRY')" > /dev/null
fi
