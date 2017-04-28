<config>
  <kdehome>kdehome</kdehome>
  <confighome>xdgconfig</confighome>
  <datahome>xdglocal</datahome>
  <envvar name="AKONADI_DISABLE_AGENT_AUTOSTART">true</envvar>
  <envvar name="PATH">${CMAKE_CURRENT_BINARY_DIR}/../:$ENV{PATH}</envvar>
  <envvar name="XDG_DATA_DIRS">${EWS_TMP_XDG_DATA_DIR}:/usr/local/share:/usr/share</envvar>
</config>
