# Hello Triangle example with SDL_gpu

Build:
- Create a CMakeUserPresets.json file that defines VCPKG_ROOT environment variable
- run `cmake --preset debug-user`
- run `cmake --build --preset debug`

Example CMakeUserPresets.json:
```
{
  "version": 2,
  "configurePresets": [
    {
      "name": "debug-user",
      "inherits": "debug",
      "environment": {
        "VCPKG_ROOT": "<path to vcpkg>"
      }
    }
  ]
}
```
