# rawprop
edit read-only properties in /dev/__properties__/ using root at runtime, no reboot necessary. This is not my original work and I have not changed anything in it. All credit goes to ezdiy who owns this repo here https://codeberg.org/ezdiy/rawprop

**About a year ago I had this idea, "what if we could change properties in /dev/__properties__/ somehow?" It turns out someone had that same idea and made it happen and it works flawlessly. I didn't bother with the Makefile or Android.mk because apparently gcc in termux is fine and can handle the includes on its own. All I did was** 
```gcc
cc -o rawprop rawprop.c -std=c11 -Wall -Wextra -DDEBUG
```
**and it compiled right away. The first one I tried was this**
```shell
./rawprop /dev/__properties__/u:object_r:bootloader_prop:s0 ro.boot.other.locked 0
```
**and sure enough when I checked the property it was now 0. Then I looked in developer options and what I saw is nothing short of a miracle. Where there had been nothing, not even a greyed out option was now the *OEM unlocking* option fully available with the switch available as well. I also added enable_oem_unlock 1 in the secure settings table so that might have helped. While this may not come as a shock to many, please keep in mind that my device is on Android 14 and although not as locked down as pretty much all Samsung devices from Android 16+, it's a North American Snapdragon model and isn't supposed to support bootloader unlocking at all without an engmode token. So while the same device in Europe has the option already, I never have so it's a pretty big deal. It remains to be seen if it actually does anything or not though and there's still a lot to figure out since I'm using the cve-2026-43499 exploit for temporary root so I fully expect it to go away after a reboot which would also mean it wouldn't be there in download mode.**

**Unfortunately I don't have the knowledge a lot of people do who have had root available for years so I'm not quite sure where to go from here that could possibly allow this to become persistent but I do believe it could be done. That would open the doors to a lot more for North American Samsung devices. I hope to see some people making good use of this instead of hoarding and being selfish assholes instead of helping the community, fuck you if that's your attitude!**

**Thank you again for the great work and for unknowingly reading my mind to bring my random thought to life ezdiy. You may have helped a lot of others!**
