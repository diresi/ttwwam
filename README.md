# ttwwam
the thing windows was always missing

Even Windows 10 sucks when it comes to virtual desktops. And yes, [GNOME](https://www.gnome.org/) also sucks in that respect.
What I want is a virtual desktop manager that works independently on each monitor.
I was indoctrinated by [Enlightment](https://www.enlightenment.org) decades ago and
later also loved [i3](https://i3wm.org/) on smaller laptop screens.

Recently I tried [Actual Window Manager](https://www.actualtools.com/) and was less than impressed.
Why does it do _${strange behaviour of the ~~day~~ hour}_?
Ok, the truth is, [Actual Window Manager](https://www.actualtools.com/) does way more,
but it still doesn't implement virtual desktops the way it should be.

IMHO.

Thus this little project was born. It's not configurable, but hey, it's rather small.

It's a Win32 application that provides a _very_ simple switcher for virtual desktops.
Somewhat similar to [i3](https://i3wm.org/) but way less functional and beautiful.

## Build Instructions
Build it with MSVC2015, run it on Win10. I haven't tried any other combination.
Also I never tried changing display resolutions. Or minimized windows.
I'm sure you get the point.

## Usage
Start the program, nothing seems to happen.
This thing waits in the background until you summon it with a suitable hotkey, `CTRL+KeyUP` that is.

It'll then show a primitive fullscreen GUI on the current monitor.
Commands start with `:`. Try them, some of them even work, like `:info` or `:quit`.

Virtual desktops (or containers, that's what they're called in the source code) can have any name, not starting with `:` though.
That's reserverd for commands.

Create a new (virtual) desktop by entering any name (remember, don't start with `:`) and pressing `Enter`.
If such a desktop exists it'll be displayed instead of the current one.
If no such desktop exists a new one will be created.

That's pretty much all so far. I already like it very much :).

## Known Issues
- The multi-monitor experience isn't very good so far (hey, I started yesterday), mostly because of the bogus window-scaling code.
- Also it seems to have problems to hide `explorer.exe` windows.
- I'm certain there's a whole bunch of other problems lurking in the dark.

## License
[MIT](https://opensource.org/licenses/MIT)
