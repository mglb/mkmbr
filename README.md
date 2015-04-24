# mkmbr

Simple tool for creating MBR partition table from command line

## Usage

```
mkmbr dev p1_start p1_sectors [p2_start p2_sectors [...]]
```

`dev` - block device path

`pX_start` - first sector number or "auto" to use next free sector

`pX_sectors` - partition's sectors count or "auto" to use all free space

