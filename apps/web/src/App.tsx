import { useEffect } from 'react';
import { ThemeProvider } from './components/ThemeProvider';
import { Editor } from './components/Editor';
import { TopBar } from './components/TopBar';
import { SidebarLibrary } from './components/SidebarLibrary';
import { Inspector } from './components/Inspector';
import { useLibraryStore } from './state/libraryStore';

export default function App() {
  const { loadInitialLibrary } = useLibraryStore();

  useEffect(() => {
    loadInitialLibrary();
  }, [loadInitialLibrary]);

  return (
    <ThemeProvider>
      <div className="flex h-screen w-full bg-slate-100 text-slate-900 dark:bg-slate-900 dark:text-slate-100">
        <SidebarLibrary />
        <div className="flex flex-1 flex-col">
          <TopBar />
          <Editor />
        </div>
        <Inspector />
      </div>
    </ThemeProvider>
  );
}
