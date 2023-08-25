FROM alpine as builder

# https://github.com/nielsfaber/scheduler-component/releases
# https://github.com/nielsfaber/scheduler-card/releases
ENV COMPONENT_VERSION=v3.2.15 \
    CARD_VERSION=v3.2.10

# install like instructions on readmes above
RUN mkdir -p /config/www/scheduler-card /config/custom_components && \
    wget -qO- https://github.com/nielsfaber/scheduler-component/releases/download/v3.2.15/scheduler.zip | \
    unzip -d /config/custom_components/scheduler - && \
    wget -qO /config/www/scheduler-card/scheduler-card.js "https://github.com/nielsfaber/scheduler-card/releases/download/${CARD_VERSION}/scheduler-card.js"

FROM lansible/home-assistant:2023.8.4
# FROM alpine

COPY --chown=1000:1000 --from=builder /config /config
COPY --chown=1000:1000 config/ /config
