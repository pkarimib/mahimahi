mahimahi: a web performance measurement toolkit

Ability to run a loss shell added that transitions from a good state into a bad state with probability `PROB_GOOD_TO_BAD` and transitions back into a good state with probability `PROB_BAD_TO_GOOD`. The lossy state itself loses packets at `BAD_LOSS_RATE`.

To have some loss in both the good and the bad state in GE mode, run:

```bash
mm-loss GE uplink|downlink BAD_LOSS_RATE PROB_BAD_TO_GOOD PROB_GOOD_TO_BAD GOOD_LOSS_RATE
```

To have no loss in the good state in bursty mode, run:
```bash
mm-loss bursty uplink|downlink LOSS_RATE PROB_BAD_TO_GOOD PROB_GOOD_TO_BAD
```

In light of this, the basic random loss shell can be spawned using
```bash
mm-loss IID uplink LOSS_RATE
```