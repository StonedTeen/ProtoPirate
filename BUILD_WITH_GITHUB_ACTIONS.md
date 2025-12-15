# Building PirateProto 2 with GitHub Actions

This document explains how to build the PirateProto 2 Flipper Zero application using GitHub Actions.

## 🤖 What is GitHub Actions?

GitHub Actions automatically builds your Flipper app in the cloud:
- No local build tools required
- No Docker setup needed
- No Codespaces required
- Automatic builds on every push/PR

## 🚀 How It Works

1. **GitHub spins up Ubuntu** - A clean virtual environment
2. **Clones Flipper firmware** - Official firmware repository
3. **Copies your app** - Places PirateProto 2 in `applications_user/`
4. **Builds a .fap** - Compiles the Flipper application package
5. **Uploads as artifact** - Makes the .fap file downloadable

## 📋 Triggering Builds

### Automatic Triggers:
- **Push to `main` or `develop` branch**
- **Pull Request to `main` branch**

### Manual Trigger:
1. Go to Actions tab in GitHub repository
2. Select "Build PirateProto 2 Flipper App" workflow
3. Click "Run workflow" → "Run workflow"

## 📦 Downloading the Built .fap

After a successful build:

1. **Go to Actions tab** in your GitHub repository
2. **Select the latest workflow run**
3. **Download artifacts**:
   - Click on "pirateproto2-fap" artifact
   - Download the ZIP file
   - Extract `pirateproto2.fap`

## 📱 Installing on Flipper Zero

1. **Download the .fap file** from GitHub Actions artifacts
2. **Copy to SD card**: `/apps/Sub-GHz/pirateproto2.fap`
3. **Reboot Flipper Zero**
4. **Launch the app**: Navigate to Sub-GHz → PirateProto 2

## 🎯 Release Builds

When you create a Git tag (release), GitHub Actions will:
- Build the app
- Create a GitHub release
- Attach the .fap file to the release

### Creating a Release:
```bash
git tag v1.0.0
git push origin v1.0.0
```

## 🔧 Build Configuration

The workflow is defined in `.github/workflows/build_flipper_app.yml`:

- **OS**: Ubuntu latest
- **Python**: 3.11
- **Firmware**: Latest Flipper Zero firmware
- **Build command**: `./fbt fap_pirateproto2`
- **Artifact retention**: 30 days

## 🐛 Troubleshooting

### Build Fails
1. Check the workflow logs in GitHub Actions
2. Look for error messages in the build steps
3. Common issues:
   - Missing files in `pirateproto2/` directory
   - Syntax errors in C code
   - Missing protocol implementations

### Download Issues
1. Make sure the build completed successfully
2. Check that artifacts are available
3. Try downloading from a different browser

## 📝 Workflow Features

### PR Comments
When a pull request is opened/updated, the workflow automatically comments with:
- Build status
- Download link for artifacts
- Installation instructions

### Automatic Releases
Tagged commits automatically create GitHub releases with the built .fap file.

### Build Matrix
The workflow can be extended to build for different Flipper Zero versions or configurations.

## 🎉 Benefits

- **Consistent builds** - Same environment every time
- **No local setup** - Works from any machine
- **Automatic testing** - Catches build errors early
- **Easy distribution** - Direct download from GitHub
- **Version control** - Built artifacts tied to commits

---

**Happy building! 🚀**
