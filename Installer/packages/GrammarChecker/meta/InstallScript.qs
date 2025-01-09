function Component()
{
    installer.setDefaultPageVisible(QInstaller.Introduction, false);
    installer.setDefaultPageVisible(QInstaller.TargetDirectory, true);
    installer.setDefaultPageVisible(QInstaller.ComponentSelection, false);
    installer.setDefaultPageVisible(QInstaller.StartMenuSelection, false);
    installer.setDefaultPageVisible(QInstaller.ReadyForInstallation, false);
    
    installer.currentPageChanged.connect(this, Component.prototype.currentPageChanged);
}

Component.prototype.createOperations = function()
{
    component.createOperations();
    
    if (installer.isInstaller())
    {
        var targetDirectory = component.userInterface("targetDirectoryForm");

        if (installer.value("os") == "win")
        {
            if (targetDirectory && targetDirectory.createShortcutOnDesktopCheckBox.checked)
            {
                component.addOperation("CreateShortcut", "@TargetDir@/GrammarChecker.exe",
                                       "@DesktopDir@/Grammar Checker.lnk",
                                       "workingDirectory=@TargetDir@",
                                       "iconPath=@TargetDir@/GrammarChecker.exe",
                                       "iconId=0", "description=Grammar Checker");
            }

            component.addOperation("CreateShortcut", "@TargetDir@/GrammarChecker.exe",
                                   "@UserStartMenuProgramsPath@/@StartMenuDir@/Grammar Checker.lnk",
                                   "workingDirectory=@TargetDir@",
                                   "iconPath=@TargetDir@/GrammarChecker.exe",
                                   "iconId=0", "description=Grammar Checker");

            component.addOperation("CreateShortcut", "@TargetDir@/maintenancetool.exe",
                                   "@UserStartMenuProgramsPath@/@StartMenuDir@/Maintenance Tool.lnk",
                                   "workingDirectory=@TargetDir@",
                                   "iconPath=@TargetDir@/GrammarChecker.exe",
                                   "iconId=0", "description=Grammar Checker");
        }
        else if (installer.value("os") == "x11")
        {
            if (targetDirectory && targetDirectory.createShortcutOnDesktopCheckBox.checked)
            {
                component.addElevatedOperation("CreateDesktopEntry",
                                               "@HomeDir@/Desktop/GrammarChecker.desktop",
                                               "Version=1.0\nType=Application\nName=Grammar Checker\nTerminal=false\nExec=@TargetDir@/GrammarChecker\nIcon=@TargetDir@/Icon.png\nCategories=Utilities");
            }

            component.addElevatedOperation("CreateDesktopEntry",
                                           "/usr/share/applications/GrammarChecker.desktop",
                                           "Version=1.0\nType=Application\nName=Grammar Checker\nTerminal=false\nExec=@TargetDir@/GrammarChecker\nIcon=@TargetDir@/Icon.png\nCategories=Utilities");
        }
    }
}

Component.prototype.currentPageChanged = function(page)
{
    try
    {
        if (installer.isInstaller() && installer.value("os") == "win")
        {
            if (page == QInstaller.TargetDirectory)
                installer.addWizardPageItem(component, "targetDirectoryForm", QInstaller.TargetDirectory);
        }
    }
    catch(e)
    {
        console.log(e);
    }
}
