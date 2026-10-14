**MPU6050 status: PASS for wiring, identity, rest accuracy and shake detection. Two tests are unverified: slow tilts and desk taps.**

|Test|Result|Evidence|
|---|---|---|
|Wiring (SDA 21, SCL 22, 3.3V, GND, AD0 to GND)|Pass [Certain]|I²C scan found only `0x68`|
|Chip identity|Pass [Certain]|WHO_AM_I = `0x68`, a genuine MPU6050|
|Rest accuracy|Pass [Certain]|`|
|Shake detection|Pass [Certain]|11 triggers for 10 shakes, weakest deviation 0.72 g against a 0.35 g threshold|
|False triggers while still|Pass [Certain]|No triggers during the idle seconds before shaking|
|Slow tilt|Unverified|Slow-sampled data stayed within ±0.1 g of baseline, but the detector was never run on tilts alone|
|Desk tap or bump|Unverified|Never run as a separate test|

**Open issues:**

- **One double trigger.** One shake fired twice, 1.5 s apart, which matches the lockout length. [Likely] I’ll fix it at integration by extending the annoyed-eyes timer while shaking continues.
- **Thresholds are untuned for the final build.** `THRESH = 0.35`, 3 peaks in 700 ms and a 1.5 s lockout work on your bare board. [Likely] Once the board is mounted in a case, retest, because the vibration will change.
- **Baseline needs a still start.** The sensor must be motionless for the first 2 seconds after reset. If you want to remove that requirement, I can add it later.