import { Bell, Settings, AlertTriangle } from 'lucide-react'
import Link from "next/link"

export function Layout({ children }: { children: React.ReactNode }) {
  return (
    <div className="min-h-screen bg-gray-50/40 dark:bg-gray-900/40">
      <header className="sticky top-0 z-50 w-full border-b bg-background/95 backdrop-blur supports-[backdrop-filter]:bg-background/60">
        <div className="container flex h-14 items-center justify-between">
          <Link href="/" className="flex items-center space-x-2">
            <div className="font-bold text-xl">AI Trading Platform</div>
          </Link>
          <div className="flex items-center space-x-4">
            <div className="text-sm text-muted-foreground">24h Volume: $1.2M</div>
            <AlertTriangle className="h-5 w-5 text-muted-foreground" />
            <Settings className="h-5 w-5 text-muted-foreground" />
          </div>
        </div>
      </header>
      <div className="container flex-1 items-start md:grid md:grid-cols-[220px_1fr] md:gap-6 lg:grid-cols-[240px_1fr] lg:gap-10">
        <aside className="fixed top-14 z-30 -ml-2 hidden h-[calc(100vh-3.5rem)] w-full shrink-0 overflow-y-auto border-r md:sticky md:block">
          <nav className="grid items-start px-4 py-4 text-sm font-medium">
            <Link
              href="/trading"
              className="flex items-center gap-3 rounded-lg px-3 py-2 text-gray-500 transition-all hover:text-gray-900 dark:text-gray-400 dark:hover:text-gray-50"
            >
              <div className="flex h-6 w-6 items-center justify-center">📊</div>
              Trading
            </Link>
            <Link
              href="/ai-agent"
              className="flex items-center gap-3 rounded-lg px-3 py-2 text-gray-500 transition-all hover:text-gray-900 dark:text-gray-400 dark:hover:text-gray-50"
            >
              <div className="flex h-6 w-6 items-center justify-center">🤖</div>
              AI Agent
            </Link>
            <Link
              href="/analytics"
              className="flex items-center gap-3 rounded-lg px-3 py-2 text-gray-500 transition-all hover:text-gray-900 dark:text-gray-400 dark:hover:text-gray-50"
            >
              <div className="flex h-6 w-6 items-center justify-center">📈</div>
              Analytics
            </Link>
            <Link
              href="/risk"
              className="flex items-center gap-3 rounded-lg px-3 py-2 text-gray-500 transition-all hover:text-gray-900 dark:text-gray-400 dark:hover:text-gray-50"
            >
              <div className="flex h-6 w-6 items-center justify-center">⚠️</div>
              Risk Management
            </Link>
            <Link
              href="/settings"
              className="flex items-center gap-3 rounded-lg px-3 py-2 text-gray-500 transition-all hover:text-gray-900 dark:text-gray-400 dark:hover:text-gray-50"
            >
              <div className="flex h-6 w-6 items-center justify-center">⚙️</div>
              Settings
            </Link>
          </nav>
        </aside>
        <main className="flex w-full flex-1 flex-col overflow-hidden p-4 md:p-6">
          {children}
        </main>
      </div>
    </div>
  )
}

