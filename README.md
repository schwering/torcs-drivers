TORCS robots
============

TORCS robots for plan recognition experiments.

[TORCS][TORCS] is a open-source racing car simulation.
We use it as driving simulation to evaluate our [plan
recognition][prGolog] system.

There are two drivers:

* `human2` is a modification of the standard `human` driver controlled
  by the keyboard. In contrast to the default `human` driver, the
  maximum speed and/or acceleration can be limited.
  The idea behind this is to allow for more realistic (i.e., not
  racing-style) driving.
* `chs` provides some drivers pursuing different styles. One of them
  just observes its environment, the others drive at various speeds on
  different lanes.

Contact: [Christoph Schwering][Schwering] (schwering at kbsg.rwth-aachen.de).


[TORCS]: http://torcs.sourceforge.net/
[prGolog]: https://github.com/schwering/prgolog

