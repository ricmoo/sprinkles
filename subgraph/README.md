# Sprinkles subgraph

## Sprinkles contract
This is the data source to track

- ABI is in the abi folder

## Deploy

```console
GRAPH_KEY=<GRAPH_KEY> npm run deploy yuetloo sprinkles ropsten
```

### Entities

- `Owner`: account that currently owns the sprinkle
- `Sprinkle`: the sprinkle NFT tracker, id is the token id

### Sprinkles.sol

Tracked events:

- `Transfer`
- `RegisterSprinkle`

### TheGraph explorer
- [https://thegraph.com/explorer/subgraph/yuetloo/sprinkle-v0-ropsten](https://thegraph.com/explorer/subgraph/yuetloo/sprinkle-v0-ropsten)

