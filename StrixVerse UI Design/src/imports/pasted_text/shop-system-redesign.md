# StrixVerse UI Prompt — Fix and Redesign the Shop System (Coins & Premium Gems)

You are the Lead UI/UX Designer and Senior C++ Client Developer for **StrixVerse**, a custom-built 2D sandbox MMO built with Modern C++20, SDL3, OpenGL 4.6, and Visual Studio 2026.

The current shop system requires a complete review and redesign. Inspect the existing implementation, fix all issues, and improve the user experience while preserving the current architecture.

---

# Primary Objective

Review, debug, and polish the entire Shop UI.

Before making any changes:

* Inspect every shop-related UI file.
* Detect broken layouts.
* Identify duplicate code.
* Verify networking integration.
* Check responsiveness.
* Fix all bugs and inconsistencies.

Do not recreate the shop unless absolutely necessary.

---

# Shop Structure

The Server Shop should contain **both Coin items and Premium Gem items**.

Instead of creating separate shop windows, organize everything into categories.

The player should always know which currency is required before purchasing an item.

---

# Currency System

Support both currencies:

### 🪙 Coins

Standard gameplay currency.

Earned from:

* Farming
* Mining
* Fishing
* Quests
* Selling items
* Trading
* NPC rewards

Used for:

* World Locks
* Seeds
* Basic Blocks
* Tools
* Crafting Stations
* Farming Supplies
* Building Materials
* Utility Items

---

### 💎 Premium Gems

Premium currency.

Obtained from:

* Purchasing Gem Packs
* Official events
* Promotional rewards
* Seasonal rewards

Used only for premium content.

Examples:

* Cosmetic Outfits
* Hats
* Hairstyles
* Wings
* Pets
* Decorative Furniture
* Decorative Blocks
* Name Change Tickets
* Inventory Expansion
* Wardrobe Expansion
* Premium Bundles

Premium Gems must never provide gameplay advantages.

---

# Shop Categories

Create the following categories.

## Coin Shop (🪙)

* Featured
* World Essentials
* Blocks
* Seeds
* Farming
* Tools
* Crafting
* Furniture
* Utility
* Miscellaneous

Every item inside these categories should display the Coin icon and Coin price.

---

## Premium Shop (💎)

* Featured Premium
* New Arrivals
* Cosmetics
* Clothing
* Hats
* Wings
* Pets
* Decorations
* Bundles
* Seasonal
* Limited Time

Every item inside these categories should display the Premium Gem icon and Gem price.

---

# Top Bar

Display:

* Shop Title
* Search Bar
* Category Filter
* Refresh Button
* Current Coin Balance
* Current Premium Gem Balance

Example:

🪙 52,450

💎 1,250

Both balances should remain visible while browsing.

---

# Item Cards

Each item card should contain:

* Item Icon
* Item Name
* Short Description
* Currency Icon
* Price
* Purchase Button

Examples:

🪙 50,000

World Lock

[ Buy ]

or

💎 350

Dragon Wings

[ Buy ]

The currency icon should immediately indicate which balance will be used.

---

# Purchase Flow

When purchasing:

1. Open a confirmation dialog.
2. Display:

   * Item Name
   * Currency Type
   * Price
   * Remaining Balance
3. Send the purchase request to the server.
4. Wait for server confirmation.
5. Update the player's balance.
6. Deliver the purchased item.
7. Show a success notification.

Never deduct Coins or Gems locally before server confirmation.

---

# UI Design

Maintain the existing StrixVerse Crystal Technology + Fantasy style.

Requirements:

* Pixel-art appearance
* Rounded windows
* Crystal blue accents
* Smooth animations
* High readability
* Responsive layout

The Coin and Premium sections should feel like parts of the same official shop while remaining visually distinct through category organization and currency icons.

---

# Networking

Verify and improve networking for:

* Shop Catalog
* Coin Purchases
* Premium Purchases
* Balance Updates
* Purchase Responses

The server must remain authoritative for:

* Prices
* Currency balances
* Purchases
* Item ownership

---

# Bug Fixes

Inspect and fix:

* Incorrect spacing
* Broken scrolling
* Layout inconsistencies
* Missing currency icons
* Wrong price display
* Purchase button bugs
* Balance synchronization issues
* UI flickering
* Memory leaks
* Duplicate widgets
* Rendering glitches

Resolve every issue before adding new features.

---

# Documentation

Update:

* `TODO.md`
* `PROJECT_STATUS.md`
* `SHOP_SYSTEM.md`
* `UI_COMPONENTS.md`
* `CHANGELOG.md`

Document:

* Files modified
* Coin Shop implementation
* Premium Shop implementation
* Networking updates
* Remaining tasks

Ensure all documentation accurately reflects the implementation to prevent misinformation.

---

# Final Report

After implementation, provide:

* Shop completion percentage
* Bugs fixed
* UI improvements
* Remaining issues
* Recommended next steps

The final Shop should present both Coin and Premium Gem items in a single, well-organized interface with clear categories, intuitive navigation, and a polished user experience.
