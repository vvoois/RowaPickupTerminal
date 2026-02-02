# Vertical Scrollbar Thumb Dragging Implementation

## Overview
Implemented full vertical scrollbar thumb dragging functionality for the GDI-based article list view.

## Changes Made

### 1. **Scrollbar State Structure** (lines 161-167)
Added a global `ScrollbarState` struct to track dragging state:
```cpp
struct ScrollbarState
{
    bool isDragging = false;      // Is user currently dragging the thumb?
    int thumbStartY = 0;          // Y position when drag started
    int scrollStartOffset = 0;    // Scroll offset when drag started
} g_scrollbarState;
```

### 2. **Global Constants** (lines 169-170)
- `SCROLLBAR_WIDTH = 15` - Width of the scrollbar in pixels
- `SCROLLBAR_THUMB_MIN_HEIGHT = 20` - Minimum thumb height

### 3. **WM_MOUSEMOVE Enhancement** (lines 1709-1733)
Detects when scrollbar is being dragged and updates scroll position:
- Calculates thumb position based on mouse Y coordinate
- Maps mouse movement to scroll offset
- Continuously updates the display during drag
- Uses conditional expressions instead of std::min/max to avoid Windows macro conflicts

### 4. **WM_LBUTTONDOWN Enhancement** (lines 1858-1942)
Handles three scroll bar interactions:
- **Thumb Drag Start**: Detects click on scrollbar thumb, captures mouse, begins drag operation
- **Page Up**: Click above thumb scrolls up by (visibleRows - 1)
- **Page Down**: Click below thumb scrolls down by (visibleRows - 1)
- Falls through to regular article row selection if click is not on scrollbar

### 5. **WM_LBUTTONUP New Handler** (lines 2000-2014)
Ends scrollbar drag operation:
- Sets `isDragging = false`
- Releases mouse capture
- Logs drag end event

### 6. **Removed Duplicate Constant** (line 1471 removed)
Removed local `const int SCROLLBAR_WIDTH = 15` from WM_PAINT to use global definition

## Key Features

? **Smooth Thumb Dragging** - Mouse-based dragging of scrollbar thumb  
? **Page Up/Down** - Click on scrollbar track to page up/down  
? **Mouse Capture** - Maintains drag even when mouse moves outside window  
? **Proper Clamping** - Scroll offset stays within valid range  
? **Synchronized Drawing** - Scrollbar position reflects current scroll state  
? **Logging** - Debug messages for drag start/end  

## Interaction Logic

```
User Action                  ? Behavior
?????????????????????????????????????????????????????
Click on thumb              ? Start dragging
Drag mouse (thumb captured) ? Scroll list smoothly
Release mouse               ? Stop dragging
Click above thumb           ? Page up
Click below thumb           ? Page down
Mouse wheel                 ? Scroll by 3 lines (existing)
Arrow keys                  ? Scroll by 1 line (existing)
```

## Technical Notes

- Scrollbar thumb height is calculated dynamically based on visible/total rows ratio
- Thumb position is proportional to scroll offset (linear mapping)
- All scrollbar calculations use integer arithmetic for efficiency
- Drag operation updates only on mouse movement (not continuous polling)
- Compatible with existing keyboard and mouse wheel scrolling

## Testing Checklist

- [ ] Click and drag scrollbar thumb up/down
- [ ] Release at various positions
- [ ] Verify list scrolls to correct position
- [ ] Click above thumb to page up
- [ ] Click below thumb to page down
- [ ] Verify thumb position matches scroll offset visually
- [ ] Test with small and large article lists
- [ ] Mouse moves fast while dragging
- [ ] Click outside window during drag (should still work)
- [ ] Double-click on rows still works
- [ ] Article selection still works while scrolling

