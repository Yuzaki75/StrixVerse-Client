You are the Lead UI/UX Designer, Game Designer, and Senior C++ Client Engineer for StrixVerse, a custom-built 2D Sandbox MMORPG developed with Modern C++20, SDL3, OpenGL 4.6, GLAD, FreeType, and Visual Studio 2026.

Your task is to design and implement the complete basic user interface for the StrixVerse client.

The UI should be modular, scalable, reusable, and production-ready. It should establish the foundation for all future game features.

Objective

Create the complete basic UI system for StrixVerse.

Before implementing anything:

Inspect every existing UI file.
Reuse existing UI components whenever possible.
Avoid duplicate implementations.
Keep the UI modular.
Update documentation after every completed feature.
Design Style

The visual style should combine:

Crystal Technology
Fantasy
Pixel Art
Clean MMORPG Interface
Minimal HUD
Modern Sandbox Game

Inspirations:

Growtopia (gameplay flow only)
Pixel Worlds (clean interface only)
Terraria (inventory usability)
Stardew Valley (simple layouts)

Do not copy copyrighted assets.

Everything must be original.

UI Theme

Theme:

Crystal Technology Fantasy

Color Palette

Primary

Crystal Blue

Secondary

Deep Navy

Accent

Gold

Background

Dark Slate

Panels

Semi-transparent dark crystal

Buttons

Rounded pixel-art buttons

Icons

Bright colorful pixel art

Animations

Smooth but lightweight

Main Screens

Create and polish the following screens.

Splash Screen

Display:

Logo
Version
Loading animation
Login Screen

Display:

Username
Password
Login
Register
Remember Me
Forgot Password

After successful login:

Authenticate automatically.

Do not ask the player to select a server.

Register Screen

Display:

Username
Email
Password
Confirm Password
Continue Screen

If the player has a previous world:

Display

Welcome back!

Last World

Continue

Change World

Continue immediately joins the previous world.

Change World opens the World Selection.

World Selection

Display

Search
Recent Worlds
Favorite Worlds
Public Worlds

Each world card contains

World Icon
World Name
Owner
Player Count
Join Button
Loading Screen

Display

Loading Bar
Loading Tips
Background Illustration
HUD

The HUD should remain clean and maximize gameplay visibility.

Top Left

Character Name
HP Bar
XP Bar

Place the HP and XP values centered inside their bars.

Remove the Energy/Stamina bar.

Top Right

Display

🪙 Coins

💎 Gems

Ping

FPS

Clicking Coins

Opens Wallet.

Clicking Gems

Opens Server Shop.

Do not display:

Server Name
Server Address
Server Region
Server Population

Bottom Center

Display

Hotbar
Selected Slot
Item Durability

Bottom Left

Display

Chat
Chat Input

Bottom Right

Display

Notifications
Interaction Prompts

Reserve space for future Quest Tracker.

Inventory

Create

Inventory Window

Features

Equipment
Backpack
Item Grid
Drag and Drop
Item Tooltip
Search
Sorting
Equipment

Display

Helmet
Chest
Legs
Boots
Main Hand
Off Hand
Back Item
Crafting

Display

Recipe Categories
Materials
Required Resources
Craft Button
Storage

Create

Chest UI

Display

Storage Slots
Transfer Buttons
Sort
Search
Wallet

Display

Coins

Gem Balance

Recent Transactions

Statistics

Server Shop

Single official shop.

Contains both currencies.

Coin Categories

Featured
Essentials
Blocks
Seeds
Farming
Tools
Furniture
Utility

Premium Gem Categories

Featured Premium
Cosmetics
Clothing
Hats
Wings
Pets
Decorations
Bundles
Seasonal

Each item displays

Icon
Name
Description
Currency Icon
Price
Buy Button
Marketplace

Separate from the Server Shop.

Player-driven economy.

Display

Listings
Search
Filters
Seller
Price
Buy Button

Players buy and sell using Coins only.

Settings

Tabs

Graphics
Audio
Controls
Gameplay
Accessibility
UI Scale
Friends

Display

Friend List
Online Status
Invite
Remove
Private Message
Guild

Display

Guild Members
Guild Chat
Guild Information
Notifications

Support

Success
Warning
Error
Information

Animated slide-in notifications.

Dialog System

Create reusable dialog windows.

Examples

Purchase Confirmation
Delete Confirmation
Disconnect
Error Messages
Tooltip System

Support

Item Information
Stats
Durability
Requirements
Description
Animation

Implement

Fade
Scale
Hover
Click
Slide
Glow

Keep animations lightweight.

Architecture

Build reusable UI components.

Examples

UIWindow
UIPanel
UIButton
UILabel
UIImage
UIProgressBar
UIScrollView
UITabView
UIGrid
UITextBox
UIDialog
UITooltip
UINotification

Do not hardcode layouts.

Networking

Ensure UI integrates with

Authentication
Inventory
Shop
Marketplace
Friends
Guild
World Selection

The server remains authoritative.

Responsiveness

Support

Different resolutions
Fullscreen
Windowed
UI scaling

Prevent overlapping elements.

Accessibility

Support

Keyboard navigation
Controller support (future)
Readable fonts
High contrast
Adjustable UI scale
Documentation

Update after every completed feature:

TODO.md
PROJECT_STATUS.md
UI_COMPONENTS.md
UI_FLOW.md
CHANGELOG.md

Every completed feature must be marked.

Every unfinished feature must remain listed.

Never remove unfinished tasks.

Final Report

After implementation, provide:

UI completion percentage
Files created
Files modified
Bugs fixed
Remaining UI tasks
Recommended next development phase

The final result should provide a polished, modular, production-ready UI foundation suitable for a long-term 2D sandbox MMORPG.