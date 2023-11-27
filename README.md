mahimahi: a web performance measurement toolkit

Ability to run a bursty loss shell added that transitions into a lossy state with probability `PROB1` and transitions back into a normal state with probability `PROB2`. The lossy state itself loses packets at `LOSS_RATE`. To use it, run:
```bash
mm-loss bursty uplink|downlink LOSS_RATE PROB1 PROB2
```

In light of this, the basic random loss shell can be spawned using
```bash
mm-loss IID uplink LOSS_RATE
```