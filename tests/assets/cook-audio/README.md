Owned 440 Hz tone, 50 ms, mono at 22,050 Hz; no game or third-party recordings.
`export.py` generated the WAV and Ogg Vorbis once using libsndfile 1.2.2 through
its [public API](https://libsndfile.github.io/libsndfile/api.html). Provenance hashes
cover the generator and both source files. CI reads these fixtures without
regeneration. Explicit reauthoring command (requires installed libsndfile):

```
python3 tests/assets/cook-audio/export.py
```

Ogg encoding may choose a new stream serial; explain any fixture update in review.
