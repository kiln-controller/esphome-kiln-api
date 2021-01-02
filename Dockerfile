ARG ARCHITECTURE
FROM lansible/home-assistant:1.0.0b6-${ARCHITECTURE}

COPY config/ /config
