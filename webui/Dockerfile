FROM node:20-alpine AS builder
WORKDIR /app
COPY package*.json .
RUN npm ci
COPY . .
RUN npm run build
RUN npm prune --production

FROM node:20-alpine
WORKDIR /app
COPY --from=builder /app/build build/
COPY --from=builder /app/node_modules node_modules/
COPY package.json .
EXPOSE 5001
ENV NODE_ENV=production PUBLIC_NO_LOCAL_DEV_ENDPOINT=1
CMD [ "node", "build" ]
