# Final Bug Report - StoneDB Engine

## Critical Bugs Identified

### Bug #1: Records Disappear After Update Moves Record to New Page
**Severity:** CRITICAL  
**Status:** UNRESOLVED  
**Description:**
When a record is updated with a larger value that requires moving to a new page, other records on the original page become inaccessible.

**Steps to Reproduce:**
1. `put k1 v1`
2. `put k2 v2`  
3. `put k3 v3`
4. `put k1 v1_longer_value` (moves to new page)
5. `scan` - only shows k1, k2 and k3 are missing
6. `get k2` - returns NOT FOUND

**Root Cause:**
When `findFreeSpaceInPage` marks the old record as deleted and searches for free space, if no suitable slot is found on the same page, the record is placed on a new page. However, the scan logic fails to properly skip the deleted record and find subsequent records on the original page.

**Impact:**
- Data loss - records become inaccessible
- Integration test fails
- Production-critical bug

**Attempted Fixes:**
1. Fixed slot reuse logic to prevent overwriting
2. Improved scan to search full page after deleted slots
3. Added byte-by-byte search for records after invalid deleted slots
4. Fixed keyToPage mapping updates

**Current Status:**
Partially fixed. `getRecord` can now find records using the fast path (keyToPage), but `scanRecords` still fails to find records after an update moves a record to a new page. The scan shows "No records found" even though individual gets work correctly. This suggests the search logic in `scanRecords` may have an edge case when iterating through pages with deleted slots at offset 0.

**Recent Fixes Applied:**
- Improved deleted record skipping in `getRecord` fast path
- Improved deleted record skipping in `open()` method when rebuilding keyToPage
- Improved search logic in `scanRecords` to scan full page when encountering invalid deleted slots
- All methods now use consistent search logic for finding records after deleted slots

**Remaining Issue:**
Scan still shows "No records found" after update that moves record to new page, even though `get` operations work correctly.

---

### Bug #2: Integration Test Fails - Delete After Update
**Severity:** HIGH  
**Status:** RELATED TO BUG #1  
**Description:**
Integration test fails when trying to delete a record after another record has been updated.

**Root Cause:**
Since Bug #1 causes records to become inaccessible after an update, the delete operation cannot find the target record.

**Impact:**
- Test suite failure
- Indicates data integrity issues

---

### Bug #3: Scan Stops Early After Deleted Slot with Invalid valueLen
**Severity:** HIGH  
**Status:** PARTIALLY FIXED  
**Description:**
When scan encounters a deleted record (keyLen=0) with invalid or zero valueLen, it breaks early instead of searching for subsequent records.

**Fix Applied:**
Added byte-by-byte search to find next valid record after invalid deleted slots. However, this doesn't fully resolve Bug #1.

---

## Non-Critical Issues

### Issue #1: Restore JSON Parsing - Trailing Whitespace
**Severity:** LOW  
**Status:** FIXED  
**Description:**
Restore command was including trailing whitespace/newlines in restored values.

**Fix:** Improved JSON parsing to skip whitespace after closing quotes.

---

### Issue #2: Infinite Loop in Slot Search
**Severity:** MEDIUM  
**Status:** FIXED  
**Description:**
Potential infinite loop when searching for free slots with invalid data.

**Fix:** Added safety checks and proper offset advancement logic.

---

## Test Results Summary

### Working Operations:
- ✅ Basic PUT operations
- ✅ Basic GET operations  
- ✅ DELETE operations
- ✅ SCAN for basic cases
- ✅ Multi-page allocation
- ✅ Persistence across restarts
- ✅ Backup/Restore (with fixes)

### Broken Operations:
- ❌ SCAN after update that moves record to new page
- ❌ GET records after update that moves record to new page
- ❌ DELETE after update
- ❌ Integration test (multiple operations in sequence)

## Recommendations

1. **Immediate:** Fix the scan logic to correctly traverse pages with deleted records
2. **Short-term:** Add comprehensive tests for update scenarios
3. **Long-term:** Consider refactoring page layout to use a more robust deletion marking scheme

## Files Modified During Debugging

- `src/storage.cpp` - Multiple fixes for slot reuse, scan logic, bounds checking
- `src/main.cpp` - Restore JSON parsing fixes
- `include/storage.hpp` - Removed duplicate declarations

