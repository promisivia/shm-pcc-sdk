# Third-Party Notices

CXL-SDK incorporates and adapts third-party research software. Those components
remain subject to their own copyright notices and license terms; the repository's
MIT license does not replace them.

The following inventory covers the principal bundled components in the current
tree. When adding or updating third-party code, update this file in the same pull
request.

| Component | Location | License or notice |
| --- | --- | --- |
| STAMP | `apps/stamp/` | [`apps/stamp/LICENSE`](apps/stamp/LICENSE) and component notices |
| BwTree | `ds/BwTree/` | [`ds/BwTree/LICENSE`](ds/BwTree/LICENSE) |
| CLHT | `ds/CLHT/` | [`ds/CLHT/LICENSE`](ds/CLHT/LICENSE) |
| HOT | `ds/HOT/` | [`ds/HOT/LICENSE`](ds/HOT/LICENSE) |
| Level Hashing | `ds/Level-Hashing/` | [`ds/Level-Hashing/LICENSE`](ds/Level-Hashing/LICENSE) |
| RadixART | `ds/RadixART/` | [`ds/RadixART/LICENSE`](ds/RadixART/LICENSE) |
| SwissTM | `stm/swisstm/` | [`stm/swisstm/LICENSE`](stm/swisstm/LICENSE) |
| TinySTM / VELOX | `stm/tinystm/` | [`stm/tinystm/LICENSE.VELOX`](stm/tinystm/LICENSE.VELOX) and bundled notices |
| TL2 | `stm/tl2/` | [`stm/tl2/LICENSE`](stm/tl2/LICENSE) |

Some components contain nested dependencies with additional `LICENSE`, `COPYING`,
`NOTICE`, `AUTHORS`, or `LEGALNOTICE` files. Redistributors must retain those
files and independently confirm that the selected components' terms are suitable
for their use. This inventory is informational and is not legal advice.
