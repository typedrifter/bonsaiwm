# BonsaiWM Context

A tiling Wayland compositor. Manages top-level surfaces, arranges them on displays, and lets users assign them to named groups and switch layouts.

## Language

### Windows & surfaces

**Client**:
A top-level surface the compositor manages, either an XDG toplevel or an X11 window. The unit of focus, tiling, and tagging.
_Avoid_: window, toplevel

**Monitor**:
A display output. Owns its own workspace state, gaps, and the Clients shown on it.

### Tagging

**Tag**:
A named group a Client belongs to; Clients can be on several Tags at once. The compositor's core organizing concept.
_Avoid_: workspace, desktop, group

**Workspace**:
A Tag as presented to external clients through the ext-workspace protocol.
_Avoid_: —

**View**:
The set of Tags a Monitor currently displays. Switching the View switches which Clients are visible and arranged on that Monitor.
_Avoid_: active tags, tagset

### Layout & placement

**Layout**:
A placement algorithm for the Clients visible on a View. Three exist: Tiling, Monocle, and Floating.
_Avoid_: mode

**Tiling**:
The default Layout: Clients fill a Master area plus a Stack, sized by the Master factor.

**Monocle**:
A Layout that shows every visible Client full-size, stacked.

**Floating**:
A Layout where Clients are placed freely and not arranged. Also the state of an individual Client that exempts it from Tiling.

**Master**:
The main window area in Tiling, holding the first few Clients in the Tiling order.
_Avoid_: primary, main

**TagState**:
The per-Monitor, per-Tag layout parameters: Master count, Master factor, and selected Layout.
_Avoid_: layout state

**Tiling order**:
The order in which a Monitor's Clients are placed within a Layout, and in which Clients are promoted to the Master area.
_Avoid_: window list, client stack

**Placement**:
The geometry (position and size) a Layout computes for each visible Client on a Monitor.
_Avoid_: geometry, box

### Focus

**Focus**:
The Client currently receiving keyboard and visual emphasis on a Monitor. Decided by pointer, stacking, and urgency.
_Avoid_: selection, active window
