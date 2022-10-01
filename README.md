# SmokelessRuntimeEFIPatcher
# These Are tool From "The Vault", that I decided to upload, so expect not fully polished product, or very slow Fixes if any

# This is not a request form
Plese don't open issue not strictly related with the tool.
Request for bios unlock are not related to the tool,
for that you can try to ask on the discord server( a bit of effort from you side is required)

Even if the tool is free, my time is not, so don't expect that I make a patch from scratch for your bios, without you putting some effort in it.

# Discaimer
**Use this at your own risk,I won’t be responsible for any damage.**

Also the code quality and the parsing engine are not that great, but were the best I could come to.


## Already baked config
https://github.com/SmokelessCPUv2/SREP-Community-Patches

# What is this
This is a simple tool to patch and Inject/Patch EFI modules at runtime, I developed this as I wasn't confortable with SPI flashing, as is not boring and require opening the laptop for every small change, as with AMD you can't flash from the OS a new BIOS, if is not signed...

# Why this exist
The real reason why this exist, is that with an update Lenovo removed the Unlock BackDoor [LenovoH2O-Unlocker](https://github.com/SmokelessCPUv2/LenovoH2O-Unlocker), so after an update I couldn't change some adv option check [Unlocking Lenovo H2O Bios](#Lenovo-BIOS-Unlock) I decided to develop a new way to do it....


# How this work
When the EFI App is booted up, it look for a file Called *SREP_Config.cfg*, containing a list of command to execute, then will execute them

# Support/Donate
If you want to donate/support please consider supportorting on [Patreon](https://www.patreon.com/SmokelessCPU)

For one one donation, you can subribe for a month to Patreon then after the biilling, unsubsribe; (I might add a paypal in future)

# How to use it
* Download the Latest zip, from the [Release Page](https://github.com/SmokelessCPUv2/SmokelessRuntimeEFIPatcher/releases/latest)

* extract in a USB, such that exist a Folder Called EFI in the USB Root,
* Create a SREP_Config.cfg and place in the root of the USB
* boot from the USB
* ??
* Profit

# SREP_Config Structure
The Config file can containg muliple batch of operation, the syntax is, 

    Op OpName1
        Argument 1
        Argument 2
        Argument n
    Op OpName2
        Argument 1
        Argument 2
        Argument n
    End

    Op OpName3
        Argument 1
        Argument 2
        Argument n
    End


# Implemented Operiation

## LoadFromFS
Load a EFI File in memory from a EFI partition, set as target
### Arguments
    * FileName : The Filename to load


## LoadFromFV
Load a EFI File in memory from the FV(Firmware Volume)/The BIOS image, set as target
### Arguments
    * SectionName : The Section to load

## Loaded
Target an already loaded Module
### Arguments
    * Name : The Name of the Loaded App to target

## Patch
Patch the previus loaded target 
### Arguments
    * Pattern : provide the Find and Replace a Patterns
    * Offset : Provide and offset from the File start, and then the Byte to replace here
    * RelNegOffset/RelPosOffset : negative/positive offset from previus Patch operation, and then the Byte to replace here
## Exec
Execute the Previus loaded Module

# To be Implemted

    [ ] Uninstall Protocol
    [ ] Lzma compressed object (very common on AMI BIOS)

# Example
This is an Example of Loading a simple EFI, and executing it:

    Op LoadFromFS APP.efi
    Op Exec
    End

This is an Example of Loading a simple EFI, replacing by pattern,and executing it

Find and replace AABBCCDDEEFF with AABBCCDDEEEE,
find and replace AABBCCDDAABB with AABBCCDDAAAA:

    Op LoadFromFS
    APP.efi
    Op Patch
    Pattern
    AABBCCDDEEFF
    AABBCCDDEEEE
    Op Patch
    Pattern
    AABBCCDDAABB
    AABBCCDDAAAA
    Op Exec
    End

This is an Example of using relative pattern

Find the pattern AABBCCDDEEFF (replace with AABBCCDDEEFF, as we want it's own start address), then write AABBCCDDAAAA, at +50 from the pattern start

    Op LoadFromFS
    APP.efi
    Op Patch
    Pattern
    AABBCCDDEEFF
    AABBCCDDEEFF
    Op Patch
    RelPosOffset
    50
    AABBCCDDAAAA
    Op Exec
    End


## Lenovo-BIOS-Unlock
Now a real example on how to use it to patch a Lenovo Legion Bios to Unlock the Advanced menu:

The Target H2O, is very simple in the regard on which form is shown...

in the H2OFormBrowserDxe there is a simple array of struct:

     struct Form
    {
        GUID FormGUID;
        uint32_t isShown;
    }

    struct Form FormList[NO_OF_FORM];

The previus cE! backdoor, was very simple, looked like this:

    if(gRS->GetVariable("cE!".....))
        for(int i=0;i<NO_OF_FORM;i++)
        {
            FormList[i].isShown=1;
        }
if the Variable didn't existed default isShow was used...

With this tool we can manually set the isShow Flag so we can replicate that behavoir...

The H2OFormBrowserDxe, is already loaded when we are able to execute SREP, so we can use the Loaded OP....

Let's say that we want to show the CBS Menu , it's guid is {B04535E3-3004-4946-9EB7-149428983053}, so it's hex representation is
 
    E33545B0043046499EB7149428983053
 
 given that is disabled, the complete Form Struct in memory will be 
 
    E33545B0043046499EB714942898305300000000

 as we appended the 4 byte uint32_t of value 0;

 we want to replace this with the 1, the little endian version of a uint32_t is
 
    01 00 00 00

so the replace string is

     E33545B0043046499EB714942898305301000000


So putting all toghether the SREP_Config.cfg file look like


    Op Loaded
    H2OFormBrowserDxe
    Op Patch
    Pattern
    E33545B0043046499EB714942898305300000000
    E33545B0043046499EB714942898305301000000
    Op End

Now we have patched the H2OFormBrowserDxe, but the Bios UI will be not loaded as we booted from a USB, but we can force it to load with

    Op LoadFromFV
    SetupUtilityApp
    Op Exec


So the Finall SREP_Config.cfg is:

    Op Loaded
    H2OFormBrowserDxe
    Op Patch
    Pattern
    E33545B0043046499EB714942898305300000000
    E33545B0043046499EB714942898305301000000
    Op End

    Op LoadFromFV
    SetupUtilityApp
    Op Exec



*Note You can't show the AOD menu with this, as it is not even loaded on non HX, cpu, you can force to load, Patching also AoDSetupDxe, but that's a topic for another day.*


You could do the same to show the PBS menu and the Advanced Menu on intel one, if you are lazy you can use the combined one AMD/Intel provided here(I might have forgot to unlock something tbh):

    Op Loaded
    H2OFormBrowserDxe
    Op Patch
    Pattern
    59B963B8C60E334099C18FD89F04022200000000
    59B963B8C60E334099C18FD89F04022201000000
    Op Patch
    Pattern
    E33545B0043046499EB714942898305300000000
    E33545B0043046499EB714942898305301000000
    Op Patch
    Pattern
    732871A65F92C64690B4A40F86A0917B00000000
    732871A65F92C64690B4A40F86A0917B01000000
    Op Patch
    Pattern
    9E76D4C6487F2A4D98E987ADCCF35CCC00000000
    9E76D4C6487F2A4D98E987ADCCF35CCC01000000
    Op End

    Op LoadFromFV
    SetupUtilityApp
    Op Exec

## Lagon Intel 2022 Config

    Op Loaded
    H2OFormBrowserDxe
    Op Patch
    Pattern
    49D592C3EB27464F8A119F5DF55A9C8B00000000
    49D592C3EB27464F8A119F5DF55A9C8B01000000
    Op Patch
    Pattern
    1AB0E0C17E60754BB8BB0631ECFAACF200000000
    1AB0E0C17E60754BB8BB0631ECFAACF201000000
    Op Patch
    Pattern
    9E76D4C6487F2A4D98E987ADCCF35CCC00000000
    9E76D4C6487F2A4D98E987ADCCF35CCC01000000
    Op Patch
    Pattern
    732871A65F92C64690B4A40F86A0917B00000000
    732871A65F92C64690B4A40F86A0917B01000000
    Op End

    Op LoadFromFV
    SetupUtilityApp
    Op Exec
