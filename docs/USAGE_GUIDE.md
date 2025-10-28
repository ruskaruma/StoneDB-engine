# StoneDB-engine Usage Guide

Complete guide to using StoneDB-engine in your projects.

## Quick Start

### Build the Database

```bash
# Build everything
mkdir build && cd build
cmake ..
make -j$(nproc)

# Or use the build script
cd ..
./build.sh
```

### Run the Database

```bash
# Default (uses stonedb.sdb in current directory)
./build/stone

# Custom database file
./build/stone --db myapp.sdb

# Batch mode (for scripts)
./build/stone --db myapp.sdb --batch

# Quiet mode (suppress logs)
./build/stone --db myapp.sdb --quiet
```

## Interactive Mode

### Starting the CLI

```bash
./build/stone --db myapp.sdb
```

You'll see:
```
StoneDB-engine v1.0.0
Database: myapp.sdb
Type 'help' for commands
stonedb> 
```

### Basic Commands

#### 1. Store Data (`put`)

```bash
stonedb> put name John
OK

stonedb> put user:1001 '{"name":"Alice","email":"alice@example.com"}'
OK

stonedb> put config:app_name MyApplication
OK
```

**Tip:** Use `key:subkey` format for namespacing (like `user:1`, `session:abc123`)

#### 2. Retrieve Data (`get`)

```bash
stonedb> get name
John

stonedb> get user:1001
{"name":"Alice","email":"alice@example.com"}

stonedb> get nonexistent
NOT FOUND
```

#### 3. Delete Data (`del`)

```bash
stonedb> del name
OK

stonedb> get name
NOT FOUND
```

#### 4. Scan All Records (`scan`)

```bash
stonedb> scan
user:1001 = {"name":"Alice","email":"alice@example.com"}
config:app_name = MyApplication
session:abc123 = {"user_id":1001,"expires":"2025-01-29"}
```

#### 5. Backup Database (`backup`)

```bash
stonedb> backup myapp_backup_20250128.json
Backup saved to myapp_backup_20250128.json (15 records)
```

#### 6. Restore Database (`restore`)

```bash
stonedb> restore myapp_backup_20250128.json
Restore completed: 15 records restored
```

**Note:** Restore adds records to the current database (doesn't replace)

## Batch Mode (Scripting)

### Single Command

```bash
echo "put status online" | ./build/stone --db status.sdb --batch
```

### Multiple Commands from File

```bash
# Create commands file
cat > commands.txt << EOF
put user:1 '{"name":"Admin"}'
put user:2 '{"name":"User"}'
put config:version 2.0
scan
EOF

# Run commands
./build/stone --db myapp.sdb --batch < commands.txt
```

### Output Only (Clean)

```bash
# Get value without logs/prompts
echo "get user:1" | ./build/stone --db myapp.sdb --batch 2>/dev/null
```

### Shell Script Examples

#### Initialize Database

```bash
#!/bin/bash
DB="myapp.sdb"

./build/stone --db "$DB" --batch << EOF
put app:name MyApplication
put app:version 1.0.0
put config:debug false
put config:max_users 1000
EOF

echo "Database initialized: $DB"
```

#### Read Configuration

```bash
#!/bin/bash
DB="myapp.sdb"

APP_NAME=$(echo "get app:name" | ./build/stone --db "$DB" --batch 2>/dev/null | tail -1)
VERSION=$(echo "get app:version" | ./build/stone --db "$DB" --batch 2>/dev/null | tail -1)

echo "App: $APP_NAME"
echo "Version: $VERSION"
```

#### Daily Backup Script

```bash
#!/bin/bash
DB="/var/lib/myapp/data.sdb"
BACKUP_DIR="/var/backups/stonedb"
DATE=$(date +%Y%m%d)

mkdir -p "$BACKUP_DIR"

echo "backup $BACKUP_DIR/backup_$DATE.json" | ./build/stone --db "$DB" --batch

# Keep only last 7 days
find "$BACKUP_DIR" -name "backup_*.json" -mtime +7 -delete

echo "Backup completed: backup_$DATE.json"
```

## Real-World Usage Patterns

### 1. Web Application Session Store

```bash
# Store session
./build/stone --db sessions.sdb --batch << EOF
put session:abc123 '{"user_id":1001,"login_time":"2025-01-28","expires":"2025-01-29"}'
EOF

# Read session
SESSION=$(echo "get session:abc123" | ./build/stone --db sessions.sdb --batch 2>/dev/null | tail -1)

# Delete expired sessions (manual)
echo "del session:abc123" | ./build/stone --db sessions.sdb --batch
```

### 2. Configuration Management

```bash
# Store configs
./build/stone --db config.sdb << EOF
put database:host localhost
put database:port 3306
put api:key secret123
put feature:new_ui_enabled true
EOF

# Read configs
HOST=$(echo "get database:host" | ./build/stone --db config.sdb --batch 2>/dev/null | tail -1)
PORT=$(echo "get database:port" | ./build/stone --db config.sdb --batch 2>/dev/null | tail -1)
```

### 3. User Data Storage

```bash
# Add users
./build/stone --db users.sdb << EOF
put user:1 '{"id":1,"name":"Alice","email":"alice@example.com","role":"admin"}'
put user:2 '{"id":2,"name":"Bob","email":"bob@example.com","role":"user"}'
put user:3 '{"id":3,"name":"Charlie","email":"charlie@example.com","role":"user"}'
EOF

# List all users
echo "scan" | ./build/stone --db users.sdb --batch | grep "user:"
```

### 4. Key-Value Cache

```bash
# Cache expensive computations
echo "put cache:expensive_result 'computed_value_12345'" | \
  ./build/stone --db cache.sdb --batch

# Check cache
echo "get cache:expensive_result" | ./build/stone --db cache.sdb --batch
```

## Multiple Database Management

### Different Databases for Different Projects

```bash
# Production database
./build/stone --db /var/lib/myapp/production.sdb

# Staging database
./build/stone --db /var/lib/myapp/staging.sdb

# Development database
./build/stone --db ~/dev/myapp.sdb

# Test database (in project directory)
./build/stone --db test_data.sdb
```

### Organize by Function

```bash
# Sessions
./build/stone --db /var/lib/myapp/sessions.sdb

# Users
./build/stone --db /var/lib/myapp/users.sdb

# Logs
./build/stone --db /var/lib/myapp/logs.sdb

# Cache
./build/stone --db /var/lib/myapp/cache.sdb
```

## Backup and Restore Workflows

### Full Backup

```bash
DB="myapp.sdb"
BACKUP="backup_$(date +%Y%m%d_%H%M%S).json"

echo "backup $BACKUP" | ./build/stone --db "$DB" --batch

echo "Backup saved: $BACKUP"
```

### Restore to New Database

```bash
# Original database
DB_ORIGINAL="myapp.sdb"

# Create backup
BACKUP="myapp_backup.json"
echo "backup $BACKUP" | ./build/stone --db "$DB_ORIGINAL" --batch

# Restore to new database
DB_NEW="myapp_restored.sdb"
echo "restore $BACKUP" | ./build/stone --db "$DB_NEW" --batch
```

### Scheduled Backups with Cron

```bash
# Add to crontab (crontab -e)
# Daily backup at 2 AM
0 2 * * * /path/to/backup_script.sh
```

Where `backup_script.sh`:
```bash
#!/bin/bash
DB="/var/lib/myapp/data.sdb"
BACKUP_DIR="/var/backups/stonedb"
DATE=$(date +%Y%m%d)

mkdir -p "$BACKUP_DIR"
echo "backup $BACKUP_DIR/backup_$DATE.json" | \
  /path/to/stone --db "$DB" --batch

# Compress old backups
find "$BACKUP_DIR" -name "backup_*.json" -mtime +7 -exec gzip {} \;

# Delete backups older than 30 days
find "$BACKUP_DIR" -name "backup_*.json.gz" -mtime +30 -delete
```

## Command-Line Options

### All Available Flags

```bash
./build/stone [OPTIONS]

Options:
  -d, --db PATH      Database file path (default: stonedb.sdb)
  -b, --batch        Batch mode (non-interactive, no prompts)
  -q, --quiet        Suppress log messages
  -h, --help         Show help message
```

### Examples

```bash
# Default behavior
./build/stone

# Custom path
./build/stone --db /var/lib/myapp.sdb

# Short flags
./build/stone -d myapp.sdb -b

# Combined flags
./build/stone --db myapp.sdb --batch --quiet
```

## Integration Examples

### Python Integration

```python
import subprocess
import json

DB_PATH = "myapp.sdb"

def stonedb_put(key, value):
    cmd = f'echo "put {key} {value}" | ./build/stone --db {DB_PATH} --batch'
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    return result.returncode == 0

def stonedb_get(key):
    cmd = f'echo "get {key}" | ./build/stone --db {DB_PATH} --batch 2>/dev/null'
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    output = result.stdout.strip().split('\n')[-1]
    return output if output != "NOT FOUND" else None

# Usage
stonedb_put("user:1", '{"name":"Alice"}')
user_data = stonedb_get("user:1")
print(user_data)
```

### Bash Function Wrapper

Add to your `~/.bashrc`:

```bash
stonedb() {
    local db="${STONEDB_DB:-stonedb.sdb}"
    if [ "$1" = "--db" ]; then
        db="$2"
        shift 2
    fi
    
    if [ -z "$1" ]; then
        ./build/stone --db "$db"
    else
        echo "$*" | ./build/stone --db "$db" --batch 2>/dev/null | tail -1
    fi
}

# Usage
export STONEDB_DB="myapp.sdb"
stonedb put name John
stonedb get name
```

### Node.js Integration

```javascript
const { execSync } = require('child_process');

const DB_PATH = 'myapp.sdb';

function stonedbPut(key, value) {
    const cmd = `echo "put ${key} ${value}" | ./build/stone --db ${DB_PATH} --batch`;
    execSync(cmd);
}

function stonedbGet(key) {
    const cmd = `echo "get ${key}" | ./build/stone --db ${DB_PATH} --batch 2>/dev/null`;
    const output = execSync(cmd, { encoding: 'utf8' });
    const lines = output.trim().split('\n');
    const result = lines[lines.length - 1];
    return result === 'NOT FOUND' ? null : result;
}

// Usage
stonedbPut('user:1', '{"name":"Alice"}');
const user = stonedbGet('user:1');
console.log(user);
```

## Tips and Best Practices

### 1. Key Naming Conventions

```bash
# Use namespaces with colons
put user:1 ...
put user:2 ...
put session:abc123 ...

# Use prefixes for types
put config:database_host ...
put config:api_key ...

# Use IDs or hashes
put cache:hash_abc123 ...
put logs:20250128_001 ...
```

### 2. JSON for Complex Data

```bash
# Store structured data as JSON
put user:1 '{"id":1,"name":"Alice","settings":{"theme":"dark"}}'

# Parse with jq
echo "get user:1" | ./build/stone --db users.sdb --batch | \
  jq '.name'
```

### 3. Data Organization

```bash
# Separate databases by domain
./build/stone --db users.sdb      # User data
./build/stone --db sessions.sdb   # Session data
./build/stone --db cache.sdb      # Cache data
./build/stone --db config.sdb     # Configuration
```

### 4. Backup Strategy

```bash
# Regular backups
0 2 * * * /path/to/daily_backup.sh

# Before major changes
echo "backup pre_migration_$(date +%Y%m%d).json" | \
  ./build/stone --db production.sdb --batch
```

### 5. Error Handling in Scripts

```bash
#!/bin/bash
set -e

DB="myapp.sdb"

# Check if database operations succeeded
if ! echo "put key value" | ./build/stone --db "$DB" --batch >/dev/null 2>&1; then
    echo "ERROR: Failed to write to database"
    exit 1
fi

# Verify write
result=$(echo "get key" | ./build/stone --db "$DB" --batch 2>/dev/null | tail -1)
if [ "$result" != "value" ]; then
    echo "ERROR: Data verification failed"
    exit 1
fi
```

## Troubleshooting

### Database File Permissions

```bash
# Check permissions
ls -l myapp.sdb

# Fix permissions
chmod 600 myapp.sdb  # Read/write for owner only
```

### Database File Location

```bash
# Find database files
find . -name "*.sdb" -o -name "*.wal"

# List size
ls -lh *.sdb *.wal
```

### Backup File Inspection

```bash
# View backup contents
cat backup.json | jq .

# Count records in backup
cat backup.json | jq '.records | length'

# Extract specific record
cat backup.json | jq '.records[] | select(.key == "user:1")'
```

## Command Reference

| Command | Description | Example |
|---------|-------------|---------|
| `put <key> <value>` | Store key-value pair | `put name John` |
| `get <key>` | Retrieve value | `get name` |
| `del <key>` | Delete key | `del name` |
| `scan` | List all records | `scan` |
| `backup <path>` | Export to JSON | `backup backup.json` |
| `restore <path>` | Import from JSON | `restore backup.json` |
| `help` | Show help | `help` |
| `quit` / `exit` | Exit database | `quit` |

## Next Steps

1. **Set up your first database**: `./build/stone --db myapp.sdb`
2. **Store some data**: `put key value`
3. **Create a backup**: `backup myapp_backup.json`
4. **Write scripts**: Use batch mode for automation
5. **Integrate**: Use in your applications via shell commands

For production deployment tips, see `PRODUCTION_README.md`.

