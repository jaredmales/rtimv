
# rtimvServer message protocol

## Assumptions

### Image Size

For maximum forward compatibility, we assume images could be up to (`2**32-1` x `2**32-1`) in size.
Thus we use `uint32_t` for the dimensions of images. 

This then requires using `uint64_t` for overall image size.

### Cubes

Cubes are supported.  The protocol always includes cube information.  Single-plane images have a z-dimension of 1.

### Image Data

The underlying images are monochromatic images.  Each pixel contains a single value corresponding to the brightness 
of a scene recorded by a camera.  

The value at each pixel is reported as a 32-bit floating point number, which includes any calibrations such as dark subtraction,
flat fielding, and masking.

## Message Definition

A message to or from `rtimvServer` is a binary message with the following format:

```
Field:     | id | payload     | crc32
Start byte:|0   |4            |4 + payload size
Content:   |cccc|-------------|xxxx
```

A message always starts with a 4-byte identifier (id), which is composed of
printable ASCII characters.  Note that in the below blanks or spaces indicate the ASCII character 0x20.

A message always ends with a 4-byte CRC-32-IEEE checksum (crc32) of the payload, calculated for everything excluding the id and crc32 itself.

The remaining payload is id specific. Note that payloads are either fixed size, or contain a field allowing calculation of the payload size.

## Configuration

After connecting to the server, the client configures the server by specifying the images to be processed for display.  

### Specify The Image Keys (cfik)

The client can configure the image keys, which specify which images are used.  
There are 5 different keys.  The configuration message consists of the
concatenations of each key (an ASCII string) preceded by its size as a `uint32_t`.

From Client to Server: This message is sent only once after making the initial connection.
To startup the server all keys that will be used must be provided at the same time.

From Server to Client: This message is sent in response to the `ik? ` query from the client.

In the below tables the notation `$sz` denotes the sum of all string sizes up to that point.  
E.g. in the field `sksz` we have `$sz = iksz + dksz + fksz + mksz`

```
Field:     | id |iksz|image-key|dksz  |dark-key |fksz  |flat-key |mksz  |mask-key |sksz  |satmask-key|crc32
Start byte:|0   |4   |8        |8+iksz|12+iksz  |12+$sz|16+$sz   |16+$sz|20+$sz   |20+$sz|24+$sz     |24+$iksz
Content:   |cfik|xxxx|c........|xxxx  |c........|xxxx  |c........|xxxx  |c........|xxxx  |c..........|xxxx
```

### Specify a Configuration File (cfcf)

```
Field:     | id |cfsz|config file name|crc32
Start byte:|0   |4   |8               |8+cfsz
Content:   |cfcf|xxxx|c...............|xxxx
```

| field            | description                       | type     | start byte  |size  | value | 
| :---:            | :---                              | :---:    | :---:       |:---: | :--- |
|  id              | message identifier                | ASCII    | 0           | 4    | 'cfcf'  |
| cfsz             | the size of the config file name  | uint32_t | 4           | 4    |      |
| config file name | the image key                     | ASCII    | 8           | var  | filename path, relative to RTIMV_CONFIG_PATH if set     |
| crc32            | CRC-32 checksum of payload        | uint32_t | 8+cfsz      | 4    |      |

### Query the Image Keys (ik? )

This simple message asks the server to report the image keys by sending the `cfik` message back to the client.

```
Field:     | id |crc-3
Start byte:|0   |4
Content:   |ik? |xxxx
```

| field   | description                                  | type     | start byte  |size  | value
| :---:   | :---                                         | :---:    | :---:       |:---: | :--- 
|  id     | message identifier                           | ASCII    | 0           | 4    | 'ik? '
| crc32  | CRC-32 checksum of payload                   | uint32_t | 4           | 4    | |

### Milkzmq Protocol Configuration

Todo: define messages for configuring milkzmq always.


## Image Display

### Image Message (imag)

This defines the message which contains an image.  Cubes are supported.

Payload Size: 42 + image-size
Message Size: 50 + image-size

```
Field:     | id |xdim|ydim|zdim|curz|frameno |sec |nsec|fm|imsize  |image         |crc32
Start byte:|0   |4   |8   |12  |16  |20      |28  |32  |36|38      |46            |46+imsize
Content:   |imag|xxxx|xxxx|xxxx|xxxx|xxxxxxxx|xxxx|xxxx|xx|xxxxxxxx|.........     |xxxx
```

| field   | description                                  | type     | start byte  |size  | value | 
| :---:   | :---                                         | :---:    | :---:       |:---: | :--- |
|  id     | message identifier                           | ASCII    | 0           | 4    | 'imag'  |
| xdim    | x dimension of the image                     | uint32_t | 4           | 4    |      |
| ydim    | y dimension of the image                     | uint32_t | 8           | 4    |      |
| zdim    | z dimension of the image cube                | uint32_t | 12          | 4    |      |
| curz    | which cube plane this image corresponds to   | uint32_t | 16          | 4    |      |
| frameno | the frame number                             | uint64_t | 20          | 8    | for ISIO this is `cnt0`|
| sec     | image acquisition time seconds               | uint32_t | 28          | 4    |      |
| nsec    | image acquisition time nanoseconds           | uint32_t | 32          | 4    |      |
| fm      | image format                                 | uint16_t | 36          | 2    | see below |
| imsize  | image size                                   | uint64_t | 38          | 8    | |
| image   | the image data                               | bytes    | 46          | var  | |
| crc32  | CRC-32 checksum of payload                   | uint32_t | 46+imsize   | 4    | |

The format `fm` field can have the following values:

| value | description      | notes
| :---: | :---             | :---
| 0     | png compression  |  
| 1     | jpeg compression |

### Image Received Response (ircv)

The server will not send the next image until the previously sent one has been acknowledged by the client.
The client acknowledges with:

```
Field:     | id |crc-3
Start byte:|0   |4
Content:   |ircv|xxxx
```

| field   | description                                  | type     | start byte  |size  | value
| :---:   | :---                                         | :---:    | :---:       |:---: | :--- 
|  id     | message identifier                           | ASCII    | 0           | 4    | 'ircv'
| crc32   | CRC-32 checksum of payload                   | uint32_t | 4           | 4    | 

### Pixel Value Query (pv? )

Message from client to server to query the value at a pixel.  

Payload size: 12 bytes
Message size: 20 bytes

```
Field:     | id |xpos|ypos|zpos|crc32
Start byte:|0   |4   |8   |12  |16
Content:   |pv? |xxxx|xxxx|xxxx|xxxx
```

| field  | description                                  | type     | start byte  |size  | value | 
| :---:  | :---                                         | :---:    | :---:       |:---: | :--- |
|  id    | message identifier                           | ASCII    | 0           | 4    | 'pv? '  |
| xpos   | x coordinate of the desired pixel            | uint32_t | 4           | 4    |      |
| ypos   | y coordinate of the desired pixel            | uint32_t | 8           | 4    |      |
| zpos   | z coordinate of the desired pixel            | uint32_t | 12          | 4    |      |
| crc32 | CRC-32 checksum of payload                   | uint32_t | 16          | 4    |      |

### Pixel Value (pv)

Message from server to client with the value at a pixel.  

Payload size: 16 bytes
Message size: 24 bytes

```
Field:     | id |xpos|ypos|zpos|frameno |pval|crc32
Start byte:|0   |4   |8   |12  |16      |24
Content:   |pv  |xxxx|xxxx|xxxx|xxxxxxxx|xxxx
```

| field   | description                       | type     | start byte  |size  | value | 
| :---:   | :---                              | :---:    | :---:       |:---: | :--- |
|  id     | message identifier                | ASCII    | 0           | 4    | 'pv  '  |
| xpos    | x coordinate of the desired pixel | uint32_t | 4           | 4    |      |
| ypos    | y coordinate of the desired pixel | uint32_t | 8           | 4    |      |
| zpos    | z coordinate of the desired pixel | uint32_t | 12          | 4    |      |
| frameno | the frame number                  | uint64_t | 16          | 8    | for ISIO this is `cnt0`|
| pval    | value of the pixel                | float32  | 24          | 4    |      |
| crc32   | CRC-32 checksum of payload        | uint32_t | 20          | 4    |      |

### Colorbar Normalization (cbnm)

Transmits the colorbar normalization (bias and contrast) settings.  From client to server this is a command to change them.  
From server to client this reports the current values.

### Colorbar Normalization Query (cbn?)

Used for the client to ask the server what the current stretch is.  Server responds with `cbst`.

### Colorbar Mode (cbmd)

Transmits the colorbar mode.

0 minmaxglobal
1 minmaxbox
2 user

### Colorbar Mode Query (cbm?)

Used for the client to ask the server what the current mode is.  Server responds with `cbmd`.

### Colorbar Stretch (cbst)

Transmits the colorbar mode.

0 linear
1 log
2 pow
3 sqrt
4 square

### Colorbar Stretch Query (cbs?)

Used for the client to ask the server what the current mode is.  Server responds with `cbst`.
